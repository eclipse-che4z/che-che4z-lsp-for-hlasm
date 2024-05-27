/*
 * Copyright (c) 2021 Broadcom.
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 *
 * Contributors:
 *   Broadcom, Inc. - initial API and implementation
 */

#include <algorithm>
#include <array>
#include <cassert>
#include <charconv>
#include <memory>
#include <regex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "diagnostic.h"
#include "diagnostic_consumer.h"
#include "document.h"
#include "lexing/logical_line.h"
#include "parse_lib_provider.h"
#include "preprocessor_options.h"
#include "preprocessor_utils.h"
#include "processing/preprocessor.h"
#include "protocol.h"
#include "range.h"
#include "semantics/source_info_processor.h"
#include "semantics/statement.h"
#include "utils/concat.h"
#include "utils/string_operations.h"
#include "utils/task.h"
#include "utils/text_matchers.h"
#include "utils/truth_table.h"
#include "utils/unicode_text.h"

namespace hlasm_plugin::parser_library::processing {

namespace {

using utils::concat;

const std::unordered_map<std::string_view, int> DFHRESP_operands = {
    { "NORMAL", 0 },
    { "ERROR", 1 },
    { "RDATT", 2 },
    { "WRBRK", 3 },
    { "EOF", 4 },
    { "EODS", 5 },
    { "EOC", 6 },
    { "INBFMH", 7 },
    { "ENDINPT", 8 },
    { "NONVAL", 9 },
    { "NOSTART", 10 },
    { "TERMIDERR", 11 },
    { "DSIDERR", 12 },
    { "FILENOTFOUND", 12 },
    { "NOTFND", 13 },
    { "DUPREC", 14 },
    { "DUPKEY", 15 },
    { "INVREQ", 16 },
    { "IOERR", 17 },
    { "NOSPACE", 18 },
    { "NOTOPEN", 19 },
    { "ENDFILE", 20 },
    { "ILLOGIC", 21 },
    { "LENGERR", 22 },
    { "QZERO", 23 },
    { "SIGNAL", 24 },
    { "QBUSY", 25 },
    { "ITEMERR", 26 },
    { "PGMIDERR", 27 },
    { "TRANSIDERR", 28 },
    { "ENDDATA", 29 },
    { "INVTSREQ", 30 },
    { "EXPIRED", 31 },
    { "RETPAGE", 32 },
    { "RTEFAIL", 33 },
    { "RTESOME", 34 },
    { "TSIOERR", 35 },
    { "MAPFAIL", 36 },
    { "INVERRTERM", 37 },
    { "INVMPSZ", 38 },
    { "IGREQID", 39 },
    { "OVERFLOW", 40 },
    { "INVLDC", 41 },
    { "NOSTG", 42 },
    { "JIDERR", 43 },
    { "QIDERR", 44 },
    { "NOJBUFSP", 45 },
    { "DSSTAT", 46 },
    { "SELNERR", 47 },
    { "FUNCERR", 48 },
    { "UNEXPIN", 49 },
    { "NOPASSBKRD", 50 },
    { "NOPASSBKWR", 51 },
    { "SEGIDERR", 52 },
    { "SYSIDERR", 53 },
    { "ISCINVREQ", 54 },
    { "ENQBUSY", 55 },
    { "ENVDEFERR", 56 },
    { "IGREQCD", 57 },
    { "SESSIONERR", 58 },
    { "SYSBUSY", 59 },
    { "SESSBUSY", 60 },
    { "NOTALLOC", 61 },
    { "CBIDERR", 62 },
    { "INVEXITREQ", 63 },
    { "INVPARTNSET", 64 },
    { "INVPARTN", 65 },
    { "PARTNFAIL", 66 },
    { "USERIDERR", 69 },
    { "NOTAUTH", 70 },
    { "VOLIDERR", 71 },
    { "SUPPRESSED", 72 },
    { "RESIDERR", 75 },
    { "NOSPOOL", 80 },
    { "TERMERR", 81 },
    { "ROLLEDBACK", 82 },
    { "END", 83 },
    { "DISABLED", 84 },
    { "ALLOCERR", 85 },
    { "STRELERR", 86 },
    { "OPENERR", 87 },
    { "SPOLBUSY", 88 },
    { "SPOLERR", 89 },
    { "NODEIDERR", 90 },
    { "TASKIDERR", 91 },
    { "TCIDERR", 92 },
    { "DSNNOTFOUND", 93 },
    { "LOADING", 94 },
    { "MODELIDERR", 95 },
    { "OUTDESCRERR", 96 },
    { "PARTNERIDERR", 97 },
    { "PROFILEIDERR", 98 },
    { "NETNAMEIDERR", 99 },
    { "LOCKED", 100 },
    { "RECORDBUSY", 101 },
    { "UOWNOTFOUND", 102 },
    { "UOWLNOTFOUND", 103 },
    { "LINKABEND", 104 },
    { "CHANGED", 105 },
    { "PROCESSBUSY", 106 },
    { "ACTIVITYBUSY", 107 },
    { "PROCESSERR", 108 },
    { "ACTIVITYERR", 109 },
    { "CONTAINERERR", 110 },
    { "EVENTERR", 111 },
    { "TOKENERR", 112 },
    { "NOTFINISHED", 113 },
    { "POOLERR", 114 },
    { "TIMERERR", 115 },
    { "SYMBOLERR", 116 },
    { "TEMPLATERR", 117 },
    { "NOTSUPERUSER", 118 },
    { "CSDERR", 119 },
    { "DUPRES", 120 },
    { "RESUNAVAIL", 121 },
    { "CHANNELERR", 122 },
    { "CCSIDERR", 123 },
    { "TIMEDOUT", 124 },
    { "CODEPAGEERR", 125 },
    { "INCOMPLETE", 126 },
    { "APPNOTFOUND", 127 },
    { "BUSY", 128 },
};

const std::unordered_map<std::string_view, int> DFHVALUE_operands = {
    { "ACQUIRED", 69 },
    { "ACQUIRING", 71 },
    { "ACTIVE", 181 },
    { "ADD", 291 },
    { "ADDABLE", 41 },
    { "ADVANCE", 265 },
    { "ALLCONN", 169 },
    { "ALLOCATED", 81 },
    { "ALLQUERY", 431 },
    { "ALTERABLE", 52 },
    { "ALTERNATE", 197 },
    { "ALTPRTCOPY", 446 },
    { "ANY", 158 },
    { "APLKYBD", 391 },
    { "APLTEXT", 393 },
    { "APPC", 124 },
    { "APPCPARALLEL", 374 },
    { "APPCSINGLE", 373 },
    { "ASATCL", 224 },
    { "ASCII7", 616 },
    { "ASCII8", 617 },
    { "ASSEMBLER", 150 },
    { "ATI", 75 },
    { "ATTENTION", 524 },
    { "AUDALARM", 395 },
    { "AUTOACTIVE", 630 },
    { "AUTOARCH", 262 },
    { "AUTOCONN", 170 },
    { "AUTOINACTIVE", 631 },
    { "AUTOPAGEABLE", 80 },
    { "AUXILIARY", 247 },
    { "AUXPAUSE", 313 },
    { "AUXSTART", 312 },
    { "AUXSTOP", 314 },
    { "BACKOUT", 192 },
    { "BACKTRANS", 397 },
    { "BASE", 10 },
    { "BATCHLU", 191 },
    { "BDAM", 2 },
    { "BELOW", 159 },
    { "BGAM", 63 },
    { "BIPROG", 160 },
    { "BISYNCH", 128 },
    { "BIT", 1600 },
    { "BLK", 47 },
    { "BLOCKED", 16 },
    { "BROWSABLE", 39 },
    { "BSAM", 61 },
    { "BTAM_ES", 62 },
    { "BUSY", 612 },
    { "C", 149 },
    { "CANCELLED", 624 },
    { "CDRDLPRT", 24 },
    { "CEDF", 370 },
    { "CICSDATAKEY", 379 },
    { "CICSEXECKEY", 381 },
    { "CICSSECURITY", 195 },
    { "CICSTABLE", 101 },
    { "CHAR", 1601 },
    { "CKOPEN", 1055 },
    { "CLEAR", 640 },
    { "CLOSED", 19 },
    { "CLOSEFAILED", 349 },
    { "CLOSELEAVE", 261 },
    { "CLOSEREQUEST", 22 },
    { "CLOSING", 21 },
    { "CMDPROT", 673 },
    { "CMDSECEXT", 207 },
    { "CMDSECNO", 205 },
    { "CMDSECYES", 206 },
    { "COBOL", 151 },
    { "COBOLII", 375 },
    { "COBOLIT", 1507 },
    { "COLDACQ", 72 },
    { "COLDQUERY", 433 },
    { "COLDSTART", 266 },
    { "COLOR", 399 },
    { "COMMIT", 208 },
    { "CONFFREE", 82 },
    { "CONFRECEIVE", 83 },
    { "CONFSEND", 84 },
    { "CONSOLE", 66 },
    { "CONTNLU", 189 },
    { "CONTROLSHUT", 623 },
    { "COPY", 401 },
    { "CPP", 624 },
    { "CREATE", 67 },
    { "CTLGALL", 632 },
    { "CTLGMODIFY", 633 },
    { "CTLGNONE", 634 },
    { "CTRLABLE", 56 },
    { "CURRENT", 260 },
    { "DB2", 623 },
    { "DEC", 46 },
    { "DEFAULT", 198 },
    { "DELAY", 637 },
    { "DELETABLE", 43 },
    { "DEST", 235 },
    { "DISABLED", 24 },
    { "DISABLING", 25 },
    { "DISCREQ", 444 },
    { "DISK1", 252 },
    { "DISK2", 253 },
    { "DISK2PAUSE", 254 },
    { "DISPATCHABLE", 228 },
    { "DPLSUBSET", 383 },
    { "DS3270", 615 },
    { "DUALCASE", 403 },
    { "DYNAMIC", 178 },
    { "EMERGENCY", 268 },
    { "EMPTY", 210 },
    { "EMPTYREQ", 31 },
    { "ENABLED", 23 },
    { "ESDS", 5 },
    { "EVENT", 334 },
    { "EXCEPT", 332 },
    { "EXCTL", 48 },
    { "EXITTRACE", 362 },
    { "EXTENDEDDS", 405 },
    { "EXTRA", 221 },
    { "EXTSECURITY", 194 },
    { "FAILEDBKOUT", 357 },
    { "FAILINGBKOUT", 358 },
    { "FCLOSE", 273 },
    { "FINALQUIESCE", 183 },
    { "FINPUT", 270 },
    { "FIRSTINIT", 625 },
    { "FIRSTQUIESCE", 182 },
    { "FIXED", 12 },
    { "FMH", 502 },
    { "FMHPARM", 385 },
    { "FOPEN", 272 },
    { "FORCE", 342 },
    { "FORCECLOSE", 351 },
    { "FORCECLOSING", 353 },
    { "FORCEPURGE", 237 },
    { "FORMFEED", 407 },
    { "FOUTPUT", 271 },
    { "FREE", 85 },
    { "FREEING", 94 },
    { "FULL", 212 },
    { "FULLAPI", 384 },
    { "FWDRECOVABLE", 354 },
    { "GENERIC", 651 },
    { "GOINGOUT", 172 },
    { "GFTSTART", 317 },
    { "GFTSTOP", 318 },
    { "HARDCOPY", 32 },
    { "HEX", 45 },
    { "HFORM", 409 },
    { "HILIGHT", 413 },
    { "HOLD", 163 },
    { "IBMCOBOL", 375 },
    { "IGNORE", 1 },
    { "IMMCLOSE", 350 },
    { "IMMCLOSING", 352 },
    { "INACTIVE", 378 },
    { "INDIRECT", 122 },
    { "INDOUBT", 620 },
    { "INFLIGHT", 621 },
    { "INITCOMPLETE", 628 },
    { "INPUT", 226 },
    { "INSERVICE", 73 },
    { "INSTART", 1502 },
    { "INSTOP", 1503 },
    { "INTACTLU", 190 },
    { "INTERNAL", 1058 },
    { "INTRA", 222 },
    { "INTSTART", 310 },
    { "INTSTOP", 311 },
    { "INVALID", 359 },
    { "IPIC", 805 },
    { "IRC", 121 },
    { "ISCMMCONV", 209 },
    { "ISOLATE", 658 },
    { "JAVA", 625 },
    { "KATAKANA", 415 },
    { "KEYED", 8 },
    { "KSDS", 6 },
    { "LE370", 377 },
    { "LIGHTPEN", 417 },
    { "LOG", 54 },
    { "LOGICAL", 216 },
    { "LPA", 165 },
    { "LU61", 125 },
    { "LUCMODGRP", 210 },
    { "LUCSESS", 211 },
    { "LUTYPE4", 193 },
    { "LUTYPE6", 192 },
    { "MAGTAPE", 20 },
    { "MAIN", 248 },
    { "MAP", 155 },
    { "MAPSET", 155 },
    { "MCHCTL", 241 },
    { "MODEL", 370 },
    { "MSRCONTROL", 419 },
    { "NEW", 28 },
    { "NEWCOPY", 167 },
    { "NOALTPRTCOPY", 447 },
    { "NOAPLKYBD", 392 },
    { "NOAPLTEXT", 394 },
    { "NOATI", 76 },
    { "NOAUDALARM", 396 },
    { "NOAUTOARCH", 263 },
    { "NOBACKTRANS", 398 },
    { "NOCEDF", 371 },
    { "NOCLEAR", 641 },
    { "NOCMDPROT", 674 },
    { "NOCOLOR", 400 },
    { "NOCOPY", 402 },
    { "NOCREATE", 68 },
    { "NOCTL", 223 },
    { "NODISCREQ", 445 },
    { "NODUALCASE", 404 },
    { "NOEMPTYREQ", 32 },
    { "NOEVENT", 335 },
    { "NOEXCEPT", 333 },
    { "NOEXCTL", 49 },
    { "NOEXITTRACE", 363 },
    { "NOEXTENDEDDS", 406 },
    { "NOFMH", 503 },
    { "NOFMHPARM", 386 },
    { "NOFORMFEED", 408 },
    { "NOHFORM", 410 },
    { "NOHILIGHT", 414 },
    { "NOHOLD", 164 },
    { "NOISOLATE", 657 },
    { "NOKATAKANA", 416 },
    { "NOLIGHTPEN", 418 },
    { "NOLOG", 55 },
    { "NOMSRCONTROL", 420 },
    { "NONAUTOCONN", 171 },
    { "NOOBFORMAT", 422 },
    { "NOOBOPERID", 388 },
    { "NOOUTLINE", 424 },
    { "NOPARTITIONS", 426 },
    { "NOPERF", 331 },
    { "NOPRESETSEC", 243 },
    { "NOPRINTADAPT", 428 },
    { "NOPROGSYMBOL", 430 },
    { "NOPRTCOPY", 449 },
    { "NOQUERY", 432 },
    { "NOREENTPROT", 681 },
    { "NORELREQ", 443 },
    { "NORMALBKOUT", 356 },
    { "NOSHUTDOWN", 289 },
    { "NOSOSI", 435 },
    { "NOSWITCH", 285 },
    { "NOSYSDUMP", 185 },
    { "NOTADDABLE", 42 },
    { "NOTALTERABLE", 53 },
    { "NOTAPPLIC", 1 },
    { "NOTCTRLABLE", 57 },
    { "NOTEXTKYBD", 437 },
    { "NOTEXTPRINT", 439 },
    { "NOTBROWSABLE", 40 },
    { "NOTBUSY", 613 },
    { "NOTDELETABLE", 44 },
    { "NOTEMPTY", 211 },
    { "NOTERMINAL", 214 },
    { "NOTFWDRCVBLE", 361 },
    { "NOTKEYED", 9 },
    { "NOTLPA", 166 },
    { "NOTPENDING", 127 },
    { "NOTPURGEABLE", 161 },
    { "NOTRANDUMP", 187 },
    { "NOTREADABLE", 36 },
    { "NOTREADY", 259 },
    { "NOTRECOVABLE", 30 },
    { "NOTREQUIRED", 667 },
    { "NOTSOS", 669 },
    { "NOTTABLE", 100 },
    { "NOTINIT", 376 },
    { "NOTTI", 78 },
    { "NOTUPDATABLE", 38 },
    { "NOUCTRAN", 451 },
    { "NOVALIDATION", 441 },
    { "NOVFORM", 412 },
    { "NOWAIT", 341 },
    { "NOZCPTRACE", 365 },
    { "OBFORMAT", 421 },
    { "OBOPERID", 387 },
    { "OBTAINING", 96 },
    { "OFF", 200 },
    { "OK", 274 },
    { "OLD", 26 },
    { "OLDCOPY", 162 },
    { "ON", 201 },
    { "OPEN", 18 },
    { "OPENAPI", 1053 },
    { "OPENING", 20 },
    { "OPENINPUT", 256 },
    { "OPENOUTPUT", 257 },
    { "OUTLINE", 423 },
    { "OUTPUT", 227 },
    { "OUTSERVICE", 74 },
    { "PAGEABLE", 79 },
    { "PARTITIONS", 425 },
    { "PARTITIONSET", 156 },
    { "PATH", 11 },
    { "PENDFREE", 86 },
    { "PENDING", 126 },
    { "PENDRECEIVE", 87 },
    { "PERF", 330 },
    { "PHASEIN", 168 },
    { "PHYSICAL", 215 },
    { "PL1", 152 },
    { "POST", 636 },
    { "PRESETSEC", 242 },
    { "PRIMARY", 110 },
    { "PRINTADAPT", 427 },
    { "PRIVATE", 174 },
    { "PROGRAM", 154 },
    { "PROGSYMBOL", 429 },
    { "PRTCOPY", 448 },
    { "PURGE", 236 },
    { "PURGEABLE", 160 },
    { "QR", 1057 },
    { "READABLE", 35 },
    { "READBACK", 209 },
    { "READONLY", 275 },
    { "READY", 258 },
    { "RECEIVE", 88 },
    { "RECOVERABLE", 29 },
    { "REENTPROT", 680 },
    { "RELEASED", 70 },
    { "RELEASING", 549 },
    { "RELREQ", 442 },
    { "REMOTE", 4 },
    { "REMOVE", 276 },
    { "REQUIRED", 666 },
    { "RESSECEXT", 204 },
    { "RESSECNO", 202 },
    { "RESSECYES", 203 },
    { "RESSYS", 208 },
    { "REVERTED", 264 },
    { "RFC3339", 647 },
    { "ROLLBACK", 89 },
    { "RPC", 1500 },
    { "RRDS", 7 },
    { "RUNNING", 229 },
    { "SCS", 614 },
    { "SDLC", 176 },
    { "SECONDINIT", 626 },
    { "SEND", 90 },
    { "SEQDISK", 18 },
    { "SESSION", 372 },
    { "SFS", 3 },
    { "SHARE", 27 },
    { "SHARED", 173 },
    { "SHUTDISABLED", 645 },
    { "SHUTENABLED", 644 },
    { "SHUTDOWN", 288 },
    { "SIGNEDOFF", 245 },
    { "SIGNEDON", 244 },
    { "SINGLEOFF", 324 },
    { "SINGLEON", 323 },
    { "SMF", 255 },
    { "SOS", 668 },
    { "SOSABOVE", 683 },
    { "SOSBELOW", 682 },
    { "SOSI", 434 },
    { "SPECIFIC", 652 },
    { "SPECTRACE", 177 },
    { "SPRSTRACE", 175 },
    { "SQL", 623 },
    { "STANTRACE", 176 },
    { "START", 635 },
    { "STARTED", 609 },
    { "STARTUP", 180 },
    { "STATIC", 179 },
    { "STOPPED", 610 },
    { "SURROGATE", 371 },
    { "SUSPENDED", 231 },
    { "SWITCH", 188 },
    { "SWITCHALL", 287 },
    { "SWITCHING", 225 },
    { "SWITCHNEXT", 286 },
    { "SYNCFREE", 91 },
    { "SYNCRECEIVE", 92 },
    { "SYNCSEND", 93 },
    { "SYSDUMP", 184 },
    { "SYSTEM", 643 },
    { "SYSTEMOFF", 320 },
    { "SYSTEMON", 319 },
    { "SYSTEM3", 161 },
    { "SYSTEM7", 2 },
    { "SYS370", 164 },
    { "SYS7BSCA", 166 },
    { "TAKEOVER", 111 },
    { "TAPE1", 250 },
    { "TAPE2", 251 },
    { "TASK", 233 },
    { "TCAM", 64 },
    { "TCAMSNA", 65 },
    { "TCEXITALL", 366 },
    { "TCEXITALLOFF", 369 },
    { "TCEXITNONE", 368 },
    { "TCEXITSYSTEM", 367 },
    { "TCONSOLE", 8 },
    { "TCPIP", 802 },
    { "TELETYPE", 34 },
    { "TERM", 234 },
    { "TERMINAL", 213 },
    { "TERMSTATUS", 606 },
    { "TEXTKYBD", 436 },
    { "TEXTPRINT", 438 },
    { "THIRDINIT", 627 },
    { "THREADSAFE", 1051 },
    { "TRANDUMP", 186 },
    { "TRANIDONLY", 452 },
    { "TTCAM", 80 },
    { "TTI", 77 },
    { "TWX33_35", 33 },
    { "T1050", 36 },
    { "T1053", 74 },
    { "T2260L", 65 },
    { "T2260R", 72 },
    { "T2265", 76 },
    { "T2740", 40 },
    { "T2741BCD", 43 },
    { "T2741COR", 42 },
    { "T2772", 130 },
    { "T2780", 132 },
    { "T2980", 134 },
    { "T3275R", 146 },
    { "T3277L", 153 },
    { "T3277R", 145 },
    { "T3284L", 155 },
    { "T3284R", 147 },
    { "T3286L", 156 },
    { "T3286R", 148 },
    { "T3600BI", 138 },
    { "T3601", 177 },
    { "T3614", 178 },
    { "T3650ATT", 186 },
    { "T3650HOST", 185 },
    { "T3650PIPE", 184 },
    { "T3650USER", 187 },
    { "T3735", 136 },
    { "T3740", 137 },
    { "T3780", 133 },
    { "T3790", 180 },
    { "T3790SCSP", 182 },
    { "T3790UP", 181 },
    { "T7770", 1 },
    { "UCTRAN", 450 },
    { "UKOPEN", 1056 },
    { "UNBLOCKED", 17 },
    { "UNDEFINED", 14 },
    { "UNDETERMINED", 355 },
    { "UNENABLED", 33 },
    { "UNENABLING", 34 },
    { "UPDATABLE", 37 },
    { "USER", 642 },
    { "USERDATAKEY", 380 },
    { "USEREXECKEY", 382 },
    { "USEROFF", 322 },
    { "USERON", 321 },
    { "USERTABLE", 102 },
    { "VALID", 360 },
    { "VALIDATION", 440 },
    { "VARIABLE", 13 },
    { "VFORM", 411 },
    { "VIDEOTERM", 64 },
    { "VSAM", 3 },
    { "VTAM", 60 },
    { "WAIT", 340 },
    { "WAITFORGET", 622 },
    { "WARMSTART", 267 },
    { "XM", 123 },
    { "XNOTDONE", 144 },
    { "XOK", 143 },
    { "ZCPTRACE", 364 },
};

const std::regex DFH_matcher(
    [](const auto& DFHRESP_operands, const auto& DFHVALUE_operands) {
        std::string DFHRESP_list;
        for (const auto& [key, value] : DFHRESP_operands)
            DFHRESP_list.append(key).append(1, '|');
        std::string DFHVALUE_list;
        for (const auto& [key, value] : DFHVALUE_operands)
            DFHVALUE_list.append(key).append(1, '|');
        // keep the empty alternatives
        return "^DFH(?:RESP[ ]*\\([ ]*(" + DFHRESP_list + ")[ ]*\\)|VALUE[ ]*\\([ ]*(" + DFHVALUE_list + ")[ ]*\\))";
    }(DFHRESP_operands, DFHVALUE_operands),
    std::regex_constants::icase);

// emulates limited variant of alternative operand parser and performs DFHRESP/DFHVALUE substitutions
// recognizes L' attribute, '...' strings and skips end of line comments
template<typename It>
class mini_parser
{
    std::string m_substituted_operands;
    std::match_results<It> m_matches;

    enum class symbol_type : unsigned char
    {
        normal,
        blank,
        apostrophe,
        comma,
        operator_symbol,
    };

    static constexpr std::array symbols = []() {
        std::array<symbol_type, std::numeric_limits<unsigned char>::max() + 1> r {};

        r[(unsigned char)' '] = symbol_type::blank;
        r[(unsigned char)'\''] = symbol_type::apostrophe;
        r[(unsigned char)','] = symbol_type::comma;

        r[(unsigned char)'*'] = symbol_type::operator_symbol;
        r[(unsigned char)'.'] = symbol_type::operator_symbol;
        r[(unsigned char)'-'] = symbol_type::operator_symbol;
        r[(unsigned char)'+'] = symbol_type::operator_symbol;
        r[(unsigned char)'='] = symbol_type::operator_symbol;
        r[(unsigned char)'<'] = symbol_type::operator_symbol;
        r[(unsigned char)'>'] = symbol_type::operator_symbol;
        r[(unsigned char)'('] = symbol_type::operator_symbol;
        r[(unsigned char)')'] = symbol_type::operator_symbol;
        r[(unsigned char)'/'] = symbol_type::operator_symbol;
        r[(unsigned char)'&'] = symbol_type::operator_symbol;
        r[(unsigned char)'|'] = symbol_type::operator_symbol;

        return r;
    }();

public:
    const std::string& operands() const& { return m_substituted_operands; }
    std::string operands() && { return std::move(m_substituted_operands); }

    class parse_and_substitute_result
    {
        std::variant<size_t, std::string_view> m_value;
        It m_last;

    public:
        explicit parse_and_substitute_result(size_t substitutions_performed, It last)
            : m_value(substitutions_performed)
            , m_last(last)
        {}
        explicit parse_and_substitute_result(std::string_view var_name, It last)
            : m_value(var_name)
            , m_last(last)
        {}

        bool error() const { return std::holds_alternative<std::string_view>(m_value); }

        std::string_view error_variable_name() const { return std::get<std::string_view>(m_value); }

        size_t substitutions_performed() const { return std::get<size_t>(m_value); }

        auto get_last() const { return m_last; }
    };

    parse_and_substitute_result parse_and_substitute(It b, It e)
    {
        m_substituted_operands.clear();
        size_t valid_dfh = 0;

        bool next_last_attribute = false;
        bool next_new_token = true;
        while (b != e)
        {
            const bool last_attribute = std::exchange(next_last_attribute, false);
            const bool new_token = std::exchange(next_new_token, false);
            const char c = *b;
            const symbol_type s = symbols[(unsigned char)c];

            switch (s)
            {
                case symbol_type::normal:
                    if (!new_token)
                        break;
                    if (c == 'L' || c == 'l')
                    {
                        if (auto n = std::next(b); n != e && *n == '\'')
                        {
                            m_substituted_operands.push_back(c);
                            m_substituted_operands.push_back('\'');
                            ++b;
                            ++b;
                            next_last_attribute = true;
                            next_new_token = true;
                            continue;
                        }
                    }
                    else if (!last_attribute && (c == 'D' || c == 'd'))
                    {
                        // check for DFHRESP/DFHVALUE expression
                        if (std::regex_search(b, e, m_matches, DFH_matcher))
                        {
                            if (m_matches[1].length() != 0)
                                m_substituted_operands.append("=F'")
                                    .append(
                                        std::to_string(DFHRESP_operands.at(utils::to_upper_copy(m_matches[1].str()))))
                                    .append("'");
                            else if (m_matches[2].length() != 0)
                                m_substituted_operands.append("=F'")
                                    .append(
                                        std::to_string(DFHVALUE_operands.at(utils::to_upper_copy(m_matches[2].str()))))
                                    .append("'");
                            else
                            {
                                if (auto c3 = *std::next(b, 3); c3 == 'R' || c3 == 'r') // indicate NULL argument error
                                    return parse_and_substitute_result("DFHRESP", m_matches.suffix().first);
                                else
                                    return parse_and_substitute_result("DFHVALUE", m_matches.suffix().first);
                            }

                            b = m_matches.suffix().first;
                            ++valid_dfh;
                            continue;
                        }
                    }
                    break;

                case symbol_type::blank:
                    // everything that follows is a comment
                    goto done;

                case symbol_type::apostrophe:
                    // read string literal
                    next_new_token = true;
                    do
                    {
                        m_substituted_operands.push_back(*b);
                        ++b;
                        if (b == e)
                            goto done;
                    } while (*b != '\'');
                    break;

                case symbol_type::comma:
                    next_new_token = true;
                    if (auto n = std::next(b); n != e && *n == ' ')
                    {
                        // skips comment at the end of the line
                        m_substituted_operands.push_back(c);
                        auto skip_line = b;
                        while (skip_line != e && utils::text_matchers::same_line(b, skip_line))
                            ++skip_line;
                        b = skip_line;
                        continue;
                    }
                    break;

                case symbol_type::operator_symbol:
                    next_new_token = true;
                    break;

                default:
                    assert(false);
                    break;
            }
            m_substituted_operands.push_back(c);
            ++b;
        }

    done:
        return parse_and_substitute_result(valid_dfh, b);
    }
};

class cics_preprocessor final : public preprocessor
{
    using ll_t = lexing::logical_line<std::string_view::iterator>;
    using ll_iterator = ll_t::const_iterator;
    using ll_range = std::pair<ll_iterator, ll_iterator>;

    ll_t m_logical_line;
    library_fetcher m_libs;
    diagnostic_op_consumer* m_diags = nullptr;
    std::vector<document_line> m_result;
    cics_preprocessor_options m_options;

    bool m_end_seen = false;
    bool m_global_macro_called = false;
    bool m_pending_prolog = false;
    bool m_pending_dfheistg_prolog = false;
    std::string_view m_pending_dfh_null_error;

    std::match_results<std::string_view::iterator> m_matches_sv;
    std::match_results<ll_iterator> m_matches_ll;

    mini_parser<ll_iterator> m_mini_parser;

    semantics::source_info_processor& m_src_proc;

public:
    cics_preprocessor(const cics_preprocessor_options& options,
        library_fetcher libs,
        diagnostic_op_consumer* diags,
        semantics::source_info_processor& src_proc)
        : m_libs(std::move(libs))
        , m_diags(diags)
        , m_options(options)
        , m_src_proc(src_proc)
    {}

    void inject_no_end_warning()
    {
        m_result.emplace_back(replaced_line { "*DFH7041I W  NO END CARD FOUND - COPYBOOK ASSUMED.\n" });
        m_result.emplace_back(replaced_line { "         DFHEIMSG 4\n" });
    }

    void inject_DFHEIGBL(bool rsect)
    {
        if (rsect)
        {
            if (m_options.leasm)
                m_result.emplace_back(replaced_line { "         DFHEIGBL ,,RS,LE          INSERTED BY TRANSLATOR\n" });
            else
                m_result.emplace_back(replaced_line { "         DFHEIGBL ,,RS,NOLE        INSERTED BY TRANSLATOR\n" });
        }
        else
        {
            if (m_options.leasm)
                m_result.emplace_back(replaced_line { "         DFHEIGBL ,,,LE            INSERTED BY TRANSLATOR\n" });
            else
                m_result.emplace_back(replaced_line { "         DFHEIGBL ,,,NOLE          INSERTED BY TRANSLATOR\n" });
        }
    }

    void inject_prolog()
    {
        m_result.emplace_back(replaced_line { "         DFHEIENT                  INSERTED BY TRANSLATOR\n" });
    }
    void inject_dfh_null_error(std::string_view variable)
    {
        m_result.emplace_back(
            replaced_line { concat("*DFH7218I S  SUB-OPERAND(S) OF '", variable, "' CANNOT BE NULL. COMMAND NOT\n") });
        m_result.emplace_back(replaced_line { "*            TRANSLATED.\n" });
        m_result.emplace_back(replaced_line { "         DFHEIMSG 12\n" });
    }
    void inject_end_code()
    {
        if (m_options.epilog)
            m_result.emplace_back(replaced_line { "         DFHEIRET                  INSERTED BY TRANSLATOR\n" });
        if (m_options.prolog)
        {
            m_result.emplace_back(replaced_line { "         DFHEISTG                  INSERTED BY TRANSLATOR\n" });
            m_result.emplace_back(replaced_line { "         DFHEIEND                  INSERTED BY TRANSLATOR\n" });
        }
    }
    void inject_DFHEISTG()
    {
        m_result.emplace_back(replaced_line { "         DFHEISTG                  INSERTED BY TRANSLATOR\n" });
    }

    bool try_asm_xopts(std::string_view input, size_t lineno)
    {
        if (input.substr(0, 5) != "*ASM ")
            return false;

        auto [line, _] = lexing::extract_line(input);
        if (m_diags && line.size() > lexing::default_ictl.end && line[lexing::default_ictl.end] != ' ')
            m_diags->add_diagnostic(diagnostic_op::warn_CIC001(range(position(lineno, 0))));

        line = line.substr(0, lexing::default_ictl.end);

        static const std::regex asm_statement(
            R"(^\*ASM[ ]+(?:[Xx][Oo][Pp][Tt][Ss]?|[Cc][Ii][Cc][Ss])[(']([A-Z, ]*)[)'])");
        static const std::regex op_sep("[ ,]+");
        static const std::unordered_map<std::string_view, std::pair<bool cics_preprocessor_options::*, bool>> opts {
            { "PROLOG", { &cics_preprocessor_options::prolog, true } },
            { "NOPROLOG", { &cics_preprocessor_options::prolog, false } },
            { "EPILOG", { &cics_preprocessor_options::epilog, true } },
            { "NOEPILOG", { &cics_preprocessor_options::epilog, false } },
            { "LEASM", { &cics_preprocessor_options::leasm, true } },
            { "NOLEASM", { &cics_preprocessor_options::leasm, false } },
        };

        std::match_results<std::string_view::const_iterator> m_regex_match;
        if (!std::regex_search(line.begin(), line.end(), m_regex_match, asm_statement)
            || m_regex_match[1].length() == 0)
            return false;

        std::string_view operands(m_regex_match[1].first, m_regex_match[1].second);
        auto opts_begin =
            std::regex_token_iterator<std::string_view::iterator>(operands.begin(), operands.end(), op_sep, -1);
        auto opts_end = std::regex_token_iterator<std::string_view::iterator>();

        for (; opts_begin != opts_end; ++opts_begin)
        {
            if (opts_begin->length() == 0)
                continue;

            std::string_view name(opts_begin->first, opts_begin->second);
            if (auto o = opts.find(name); o != opts.end())
                (m_options.*o->second.first) = o->second.second;
        }

        return true;
    }

    bool process_asm_statement(std::string_view type, std::string_view sect_name)
    {
        switch (type.front())
        {
            case 'D':
                if (!std::exchange(m_global_macro_called, true))
                    inject_DFHEIGBL(false);
                if (type.starts_with("DFHE"))
                    return false;
                // DSECT otherwise
                if (sect_name != "DFHEISTG")
                    return false;
                m_pending_dfheistg_prolog = m_options.prolog;
                break;

            case 'S':
            case 'C':
                if (!std::exchange(m_global_macro_called, true))
                    inject_DFHEIGBL(false);
                m_pending_prolog = m_options.prolog;
                break;

            case 'R':
                m_global_macro_called = true;
                inject_DFHEIGBL(true);
                m_pending_prolog = m_options.prolog;
                break;

            case 'E':
                m_end_seen = true;
                inject_end_code();
                break;

            default:
                assert(false);
                break;
        }
        return true;
    }

    static constexpr const lexing::logical_line_extractor_args cics_extract { 1, 71, 2, false, false };

    static constexpr size_t valid_cols = 1 + lexing::default_ictl.end - (lexing::default_ictl.begin - 1);
    static auto create_line_preview(std::string_view input)
    {
        return utils::utf8_substr(lexing::extract_line(input).first, lexing::default_ictl.begin - 1, valid_cols);
    }

    static bool is_ignored_line(std::string_view line, size_t line_len_chars)
    {
        if (line.empty() || line.front() == '*' || line.starts_with(".*"))
            return true;

        // apparently lines full of characters are ignored
        if (line_len_chars == valid_cols && line.find(' ') == std::string_view::npos)
            return true;

        return false;
    }

    bool process_line_of_interest(std::string_view line)
    {
        static const std::regex line_of_interest("^([^ ]*)[ ]+(START|CSECT|RSECT|DSECT|DFHEIENT|DFHEISTG|END)(?= |$)");

        return (std::regex_search(line.begin(), line.end(), m_matches_sv, line_of_interest)
            && process_asm_statement(std::string_view(m_matches_sv[2].first, m_matches_sv[2].second),
                std::string_view(m_matches_sv[1].first, m_matches_sv[1].second)));
    }

    struct label_info
    {
        size_t byte_length;
        size_t char_length;
    };

    void echo_text(const label_info& li)
    {
        // print lines, remove continuation character and label on the first line
        bool first_line = true;
        for (const auto& l : m_logical_line.segments)
        {
            std::string buffer;
            auto it = l.begin;
            utils::utf8_next(it, cics_extract.end, l.end);
            buffer.append(l.begin, it);
            utils::utf8_next(it, 1, l.end);

            if (it != l.end)
                buffer.append(" ").append(it, l.end);

            if (first_line)
                buffer.replace(0, li.byte_length, li.char_length, ' ');

            buffer[0] = '*';
            buffer.append("\n");
            m_result.emplace_back(replaced_line { std::move(buffer) });
            first_line = false;
        }
    }

    template<typename It>
    static std::string generate_label_fragment(It label_b, It label_e, const label_info& li)
    {
        if (li.char_length <= 8)
            return std::string(label_b, label_e) + std::string(9 - li.char_length, ' ');
        else
            return std::string(label_b, label_e) + " DS 0H\n";
    }

    template<typename It>
    void inject_call(It label_b, It label_e, const label_info& li)
    {
        if (li.char_length <= 8)
            m_result.emplace_back(
                replaced_line { generate_label_fragment(label_b, label_e, li) + "DFHECALL =X'0E'\n" });
        else
        {
            m_result.emplace_back(replaced_line { generate_label_fragment(label_b, label_e, li) });
            m_result.emplace_back(replaced_line { "         DFHECALL =X'0E'\n" });
        }
        // TODO: generate correct calls
    }

    void process_exec_cics(const std::match_results<ll_iterator>& matches)
    {
        auto label_b = matches[1].first;
        auto label_e = matches[1].second;
        label_info li {
            (size_t)std::ranges::distance(label_b, label_e),
            (size_t)std::count_if(label_b, label_e, [](unsigned char c) { return (c & 0xc0) != 0x80; }),
        };
        echo_text(li);
        inject_call(label_b, label_e, li);
    }

    static bool is_command_present(const std::match_results<ll_iterator>& matches) { return matches[3].matched; }

    bool try_exec_cics(preprocessor::line_iterator& it,
        const preprocessor::line_iterator& end,
        const std::optional<size_t>& potential_lineno)
    {
        static const std::regex exec_cics("^([^ ]*)[ ]+([eE][xX][eE][cC][ ]+[cC][iI][cC][sS])(?:[ ]+(\\S+))?(?= |$)");

        it = extract_nonempty_logical_line(m_logical_line, it, end, cics_extract);
        bool exec_cics_continuation_error = false;
        if (m_logical_line.continuation_error)
        {
            exec_cics_continuation_error = true;
            // keep 1st line only
            m_logical_line.segments.erase(m_logical_line.segments.begin() + 1, m_logical_line.segments.end());
        }

        if (!std::regex_search(m_logical_line.begin(), m_logical_line.end(), m_matches_ll, exec_cics))
            return false;

        auto lineno = potential_lineno.value_or(0);
        if (is_command_present(m_matches_ll))
        {
            process_exec_cics(m_matches_ll);

            if (exec_cics_continuation_error)
            {
                if (m_diags)
                    m_diags->add_diagnostic(diagnostic_op::warn_CIC001(range(position(lineno, 0))));
                m_result.emplace_back(replaced_line { "*DFH7080I W  CONTINUATION OF EXEC COMMAND IGNORED.\n" });
                m_result.emplace_back(replaced_line { "         DFHEIMSG 4\n" });
            }
        }
        else
        {
            if (m_diags)
                m_diags->add_diagnostic(diagnostic_op::warn_CIC003(range(position(lineno, 0))));
            m_result.emplace_back(replaced_line { "*DFH7237I S  INCORRECT SYNTAX AFTER 'EXEC CICS'. COMMAND NOT\n" });
            m_result.emplace_back(replaced_line { "*            TRANSLATED.\n" });
            m_result.emplace_back(replaced_line { "         DFHEIMSG 12\n" });
        }

        if (potential_lineno)
        {
            static const stmt_part_ids part_ids { 1, { 2, 3 }, (size_t)-1, std::nullopt };
            auto matches = make_preproc_matches<3>(m_matches_ll);
            auto stmt = get_preproc_statement<semantics::preprocessor_statement_si>(
                std::span(matches.cbegin(), matches.cend()), part_ids, lineno, true, 1);
            do_highlighting(*stmt, m_logical_line, m_src_proc, 1);
            set_statement(std::move(stmt));
        }

        return true;
    }

    auto try_substituting_dfh(const ll_range& label, const ll_range& instruction, const ll_range& rest)
    {
        auto events = m_mini_parser.parse_and_substitute(rest.first, rest.second);
        if (!events.error() && events.substitutions_performed() > 0)
        {
            const auto& [label_b, label_e] = label;
            label_info li {
                (size_t)std::ranges::distance(label_b, label_e),
                (size_t)std::count_if(label_b, label_e, [](unsigned char c) { return (c & 0xc0) != 0x80; }),
            };

            echo_text(li);

            std::string text_to_add(instruction.first, instruction.second);
            if (auto instr_len = utils::utf8_substr(text_to_add).char_count; instr_len < 4)
                text_to_add.append(4 - instr_len, ' ');
            text_to_add.append(1, ' ').append(m_mini_parser.operands());
            text_to_add.insert(0, generate_label_fragment(label_b, label_e, li));

            std::string_view prefix;
            std::string_view t = text_to_add;

            size_t line_limit = 72;
            while (true)
            {
                auto part = utils::utf8_substr(t, 0, line_limit);
                t.remove_prefix(part.str.size());

                if (t.empty())
                {
                    m_result.emplace_back(replaced_line { concat(prefix, part.str, "\n") });
                    break;
                }
                else
                    m_result.emplace_back(replaced_line { concat(prefix, part.str, "*\n") });

                prefix = "               ";
                line_limit = 56;
            }
        }

        return events;
    }

    bool try_dfh_lookup(preprocessor::line_iterator& it,
        const preprocessor::line_iterator& end,
        const std::optional<size_t>& potential_lineno)
    {
        std::array<std::pair<ll_iterator, ll_iterator>, 5> matches;

        namespace m = utils::text_matchers;
        static constexpr auto first_letter =
            utils::create_truth_table("$#@abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
        static constexpr auto next_letter =
            utils::create_truth_table("_$#@abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");

        const auto line_matcher = m::seq(m::capture(matches[2], m::star(m::not_char_matcher(" "))),
            m::space_matcher<false, false>(),
            m::capture(matches[3], m::seq(m::byte_matcher(first_letter), m::star(m::byte_matcher(next_letter)))),
            m::space_matcher<false, false>(),
            m::capture(matches[4], m::seq(m::not_char_matcher(" "), m::skip_to_end())));

        bool ret_val = false;
        auto lineno = potential_lineno.value_or(0);
        it = extract_nonempty_logical_line(m_logical_line, it, end, lexing::default_ictl);

        if (m_logical_line.continuation_error)
        {
            if (m_diags)
                m_diags->add_diagnostic(diagnostic_op::warn_CIC001(range(position(lineno, 0))));
        }
        else if (auto b = m_logical_line.begin(); line_matcher(b, m_logical_line.end()))
        {
            auto r = try_substituting_dfh(matches[2], matches[3], matches[4]);
            if (r.error())
            {
                if (m_diags)
                    m_diags->add_diagnostic(
                        diagnostic_op::warn_CIC002(range(position(lineno, 0)), r.error_variable_name()));
                m_pending_dfh_null_error = r.error_variable_name();
            }
            else if (r.substitutions_performed() > 0)
                ret_val = true;

            if ((r.error() || r.substitutions_performed() > 0) && potential_lineno)
            {
                static const stmt_part_ids part_ids { 1, { 2 }, 3, (size_t)-1 };
                matches[0] = { r.get_last(), m_logical_line.end() };
                matches[1] = { m_logical_line.begin(), r.get_last() };
                matches[4].second = r.get_last();

                auto stmt = get_preproc_statement<semantics::preprocessor_statement_si>(
                    std::span(matches.cbegin(), matches.cend()), part_ids, lineno, false);
                do_highlighting(*stmt, m_logical_line, m_src_proc);
                set_statement(std::move(stmt));
            }
        }

        return ret_val;
    }

    static bool is_process_line(std::string_view s)
    {
        static constexpr const std::string_view PROCESS = "*PROCESS ";
        return s.size() >= PROCESS.size()
            && std::equal(PROCESS.begin(), PROCESS.end(), s.begin(), [](unsigned char l, unsigned char r) {
                   return l == toupper(r);
               });
    }

    void do_general_injections()
    {
        if (std::exchange(m_pending_prolog, false))
            inject_prolog();
        if (std::exchange(m_pending_dfheistg_prolog, false))
            inject_DFHEISTG();
        if (!m_pending_dfh_null_error.empty())
            inject_dfh_null_error(std::exchange(m_pending_dfh_null_error, std::string_view()));
    }

    // Inherited via preprocessor
    [[nodiscard]] utils::value_task<document> generate_replacement(document doc) override
    {
        reset();
        m_result.clear();
        m_result.reserve(doc.size());

        auto it = doc.begin();
        const auto end = doc.end();

        bool skip_continuation = false;
        bool asm_xopts_allowed = true;
        while (it != end)
        {
            const auto text = it->text();
            if (skip_continuation)
            {
                m_result.emplace_back(*it++);
                skip_continuation = is_continued(text);
                continue;
            }

            do_general_injections();

            const auto lineno = it->lineno(); // TODO: preprocessor chaining

            if (asm_xopts_allowed && is_process_line(text))
            {
                m_result.emplace_back(*it++);
                // ignores continuation
                continue;
            }

            if (asm_xopts_allowed && try_asm_xopts(it->text(), lineno.value_or(0)))
            {
                m_result.emplace_back(*it++);
                // ignores continuation
                continue;
            }

            asm_xopts_allowed = false;

            if (auto [line, line_len_chars, _, __] = create_line_preview(text);
                is_ignored_line(line, line_len_chars) || process_line_of_interest(line))
            {
                m_result.emplace_back(*it++);
                skip_continuation = is_continued(text);
                continue;
            }

            const auto it_backup = it;
            if (try_exec_cics(it, end, lineno))
                continue;

            it = it_backup;
            if (try_dfh_lookup(it, end, lineno))
                continue;

            it = it_backup;

            m_result.emplace_back(*it++);
            skip_continuation = is_continued(text);
        }

        do_general_injections();
        if (!std::exchange(m_end_seen, true) && !asm_xopts_allowed) // actual code encountered
            inject_no_end_warning();

        co_return document(std::move(m_result));
    }

    cics_preprocessor_options current_options() const { return m_options; }
};

} // namespace

std::unique_ptr<preprocessor> preprocessor::create(const cics_preprocessor_options& options,
    library_fetcher libs,
    diagnostic_op_consumer* diags,
    semantics::source_info_processor& src_proc)
{
    return std::make_unique<cics_preprocessor>(options, std::move(libs), diags, src_proc);
}

namespace test {
cics_preprocessor_options test_cics_current_options(const preprocessor& p)
{
    return static_cast<const cics_preprocessor&>(p).current_options();
}

std::pair<int, std::string> test_cics_miniparser(const std::vector<std::string_view>& list)
{
    lexing::logical_line<std::string_view::iterator> ll;
    std::ranges::transform(list, std::back_inserter(ll.segments), [](std::string_view s) {
        return lexing::logical_line_segment<std::string_view::iterator> {
            s.begin(), s.begin(), s.end(), s.end(), s.end()
        };
    });

    mini_parser<lexing::logical_line<std::string_view::iterator>::const_iterator> p;
    std::pair<int, std::string> result;

    auto p_s = p.parse_and_substitute(ll.begin(), ll.end());
    if (p_s.error())
        result.first = -1;
    else
    {
        result.first = (int)p_s.substitutions_performed();
        result.second = std::move(p).operands();
    }

    return result;
}
} // namespace test

} // namespace hlasm_plugin::parser_library::processing
