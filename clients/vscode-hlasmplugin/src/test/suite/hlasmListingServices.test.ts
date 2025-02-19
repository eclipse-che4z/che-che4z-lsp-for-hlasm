/*
 * Copyright (c) 2023 Broadcom.
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

import * as assert from 'assert';
import * as vscode from 'vscode';
import { createListingServices } from '../../hlasmListingServices';

const sampleListing = `
                                                                                                                       
                                         High Level Assembler Option Summary                   (PTF UI93396)   Page    1
                                                                                            HLASM R6.0  2023/12/21 08.41
  No Overriding ASMAOPT Parameters                                                                                      
  Overriding Parameters-  OBJECT,NODECK,XREF(FULL),LINECOUNT(5),LIST(121)                                               
  No Process Statements                                                                                                 
                                                                                                                        
** ASMA400W Error in invocation parameter - ,LINECOUNT(5)                                                               
                                                                                                                        
  Options for this Assembly                                                                                             
                    NOADATA                                                                                             
                      ALIGN                                                                                             
                    NOASA                                                                                               
                      ASCII(00819)                                                                                      
                      BATCH                                                                                             
                      CA(LOCAL)                                                                                         
                      CE(LOCAL)                                                                                         
                      CODEPAGE(01148)                                                                                   
                    NOCOMPAT                                                                                            
                      CU(LOCAL)                                                                                         
                    NODATAMAP                                                                                           
                    NODBCS                                                                                              
3 PARM/OPTION       NODECK                                                                                              
                      DXREF                                                                                             
                      EBCDIC(00037)                                                                                     
                      ESD                                                                                               
                    NOEXIT                                                                                              
                      FAIL(NOMSG,NOMNOTE,MAXERRS(500))                                                                  
                      FLAG(0,ALIGN,CONT,EXLITW,NOIMPLEN,NOLONGER,NOPAGE0,PUSH,RECORD,RENT,NOSIGNED,NOSUBSTR,NOTRUNC,    
                           USING0)                                                                                      
                    NOFOLD                                                                                              
                    NOGOFF                                                                                              
                      ILMA                                                                                              
                    NOINFO                                                                                              
                      LANGUAGE(EN)                                                                                      
                    NOLIBMAC                                                                                            
                      LINECOUNT(60)                                                                                     
3 PARM/OPTION         LIST(121)                                                                                         
                      MACHINE(,NOLIST)                                                                                  
                      MXREF(SOURCE)                                                                                     
3 PARM/OPTION         OBJECT                                                                                            
                      OPTABLE(UNI,NOLIST)                                                                               
                    NOPCONTROL                                                                                          
                    NOPESTOP                                                                                            
                    NOPROFILE                                                                                           
                    NORA2                                                                                               
                    NORENT                                                                                              
                      RLD                                                                                               
                      RXREF                                                                                             
                      SECTALGN(8)                                                                                       
                      SIZE(MAX)                                                                                         
                    NOSUPRWARN                                                                                          
                      SYSPARM()                                                                                         
                    NOTERM                                                                                              
                    NOTEST                                                                                              
                      THREAD                                                                                            
                    NOTRANSLATE                                                                                         
                      TYPECHECK(MAGNITUDE,REGISTER,SIGNED)                                                              
                                                                                                                        
                                         High Level Assembler Option Summary                   (PTF UI93396)   Page    2
                                                                                            HLASM R6.0  2023/12/21 08.41
                      UNICODE(01200)                                                                                    
                      USING(NOLIMIT,MAP,WARN(15))                                                                       
                    NOWORKFILE                                                                                          
3 PARM/OPTION         XREF(FULL)                                                                                        
                                                                                                                        
  No Overriding DD Names                                                                                                
                                                                                                                        
                                              External Symbol Dictionary                                       Page    3
Symbol   Type   Id     Address  Length   Owner Id Flags Alias-of                            HLASM R6.0  2023/12/21 08.41
C         SD 00000001 00000000 00000008             00                                                                  
                                                                                                                        
                                                                                                               Page    4
  Active Usings: None                                                                                                   
  Loc  Object Code    Addr1 Addr2  Stmt   Source Statement                                  HLASM R6.0  2023/12/21 08.41
000000                00000 00008     1 C        CSECT                                                                  
                 R:1  00000           2          USING D,1                                                              
000000 4100 1000            00000     3          LA    0,TEST                                                           
000004 4100 1004            00004     4          LA                                                        0,TEX        
                                                       ST2                                                              
000000                00000 00008     5 D        DSECT                                                                  
000000                                6 TEST     DS    A                                                                
000004                                7 TEST2    DS    A                                                                
                                      8          END                                                            00032801
                                                                                                                        
                               Ordinary Symbol and Literal Cross Reference                                     Page    5
Symbol   Length   Value     Id    R Type Asm  Program   Defn References                     HLASM R6.0  2023/12/21 08.41
C             1 00000000 00000001     J                    1                                                            
D             1 00000000 FFFFFFFF     J                    5    2U                                                      
TEST          4 00000000 FFFFFFFF     A  A                 6    3                                                       
TEST2         4 00000004 FFFFFFFF     A  A                 7    4                                                       
                                                                                                                        
                                                Dsect Cross Reference                                          Page    6
Dsect     Length      Id       Defn                                                         HLASM R6.0  2023/12/21 08.41
D        00000008  FFFFFFFF       5                                                                                     
                                                                                                                        
                                                      Using Map                                                Page    7
                                                                                            HLASM R6.0  2023/12/21 08.41
  Stmt  -----Location----- Action ----------------Using----------------- Reg Max     Last Label and Using Text          
          Count      Id           Type          Value    Range     Id        Disp    Stmt                               
     2  00000000  00000001 USING  ORDINARY    00000000 00001000 FFFFFFFF   1 00004      4 D,1                           
                                                                                                                        
                                   General Purpose Register Cross Reference                                    Page    8
 Register  References (M=modified, B=branch, U=USING, D=DROP, N=index)                      HLASM R6.0  2023/12/21 08.41
    0(0)       3M    4M                                                                                                 
    1(1)       2U                                                                                                       
    2(2)    (no references identified)                                                                                  
    3(3)    (no references identified)                                                                                  
    4(4)    (no references identified)                                                                                  
    5(5)    (no references identified)                                                                                  
    6(6)    (no references identified)                                                                                  
    7(7)    (no references identified)                                                                                  
    8(8)    (no references identified)                                                                                  
    9(9)    (no references identified)                                                                                  
   10(A)    (no references identified)                                                                                  
   11(B)    (no references identified)                                                                                  
   12(C)    (no references identified)                                                                                  
   13(D)    (no references identified)                                                                                  
   14(E)    (no references identified)                                                                                  
   15(F)    (no references identified)                                                                                  
                                                                                                                        
                                  Diagnostic Cross Reference and Assembler Summary                             Page    9
                                                                                            HLASM R6.0  2023/12/21 08.41
     No Statements Flagged in this Assembly                                                                             
HIGH LEVEL ASSEMBLER, 5696-234, RELEASE 6.0, PTF UI93396                                                                
SYSTEM: z/OS 02.05.00              JOBNAME: XXXXXXXX    STEPNAME: STEP1      PROCSTEP: C                                
Unicode Module:   ASMA047C   From Page     1148   To Page    17584           ECECP: International 1                     
Data Sets Allocated for this Assembly                                                                                   
 Con DDname   Data Set Name                                Volume  Member                                               
  P1 SYSIN    XXXXXXX.XXXXXXXX.XXXXXXXX.D0000101.?                                                                      
  L1 SYSLIB   ASMA.SASMMAC1                                VOLUME                                                       
  L2          SYS1.MACLIB                                  VOLUME                                                       
     SYSLIN   XXXXXXX.XXXXXXXX.XXXXXXXX.D0000104.OBJ                                                                    
     SYSPRINT XXXXXXX.XXXXXXXX.XXXXXXXX.D0000102.?                                                                      
                                                                                                                        
  26868K allocated to Buffer Pool       Storage required     200K                                                       
      9 Primary Input Records Read            0 Library Records Read                  0 Work File Reads                 
      0 ASMAOPT Records Read                141 Primary Print Records Written         0 Work File Writes                
      3 Object Records Written                0 ADATA Records Written                                                   
Assembly Start Time: 08.41.00 Stop Time: 08.41.00 Processor Time: 01.11.34.9672                                         
Return Code 004                                                                                                         
`;

suite('Language services for listings', () => {
    let services: ReturnType<typeof createListingServices>;
    let document: vscode.TextDocument;
    let listings: ReturnType<ReturnType<typeof createListingServices>['handleListingContent']>;

    suiteSetup(async () => {
        services = createListingServices();
        document = await vscode.workspace.openTextDocument({ language: 'hlasmListing', content: sampleListing });
        listings = services.handleListingContent(document);
        assert.ok(listings);
    })

    suiteTeardown(async () => {
        services.releaseListingContent(document);
    })

    test('Validate listing structure', async () => {
        assert.strictEqual(listings?.length, 1);

        const l = listings[0];

        assert.deepStrictEqual(l.options, { start: 2, end: 69 });
        assert.deepStrictEqual(l.externals, { start: 69, end: 73 });
        assert.deepStrictEqual(l.codeSections, [{ start: 73, end: 86, title: '', codeStart: 76, dsect: false, firstStmtNo: 1 }]);
        assert.strictEqual(l.relocations, undefined);
        assert.deepStrictEqual(l.ordinary, { start: 86, end: 93 });
        assert.strictEqual(l.macro, undefined);
        assert.deepStrictEqual(l.dsects, { start: 93, end: 97 });
        assert.deepStrictEqual(l.usings, { start: 97, end: 103 });
        assert.deepStrictEqual(l.registers, { start: 103, end: 122 });
        assert.deepStrictEqual(l.summary, { start: 122, end: 142 });

    })

    test('Definition in code block', async () => {
        const def1 = services.provideDefinition(document, new vscode.Position(79, 110));
        const def2 = services.provideDefinition(document, new vscode.Position(80, 56));

        assert.deepStrictEqual(def1, def2);
        assert.strictEqual(def1?.length, 1);
        assert.deepStrictEqual(def1[0].range.start.line, 83);
    });

    test('Definition outside of code block', async () => {
        const def = services.provideDefinition(document, new vscode.Position(101, 90));

        assert.strictEqual(def?.length, 1);
        assert.deepStrictEqual(def[0].range.start.line, 81);
    });

    test('References', async () => {
        const ref = services.provideReferences(document, new vscode.Position(101, 90), { includeDeclaration: false });

        assert.strictEqual(ref?.length, 1);
        assert.deepStrictEqual(ref[0].range.start.line, 77);
    });

    test('References with definition', async () => {
        const ref = services.provideReferences(document, new vscode.Position(101, 90), { includeDeclaration: true });

        assert.strictEqual(ref?.length, 2);
        assert.deepStrictEqual(ref?.map(x => x.range.start.line).sort((l, r) => l - r), [77, 81]);
    });

    test('Hover', async () => {
        const h = services.provideHover(document, new vscode.Position(101, 90));

        const md = h?.contents[0];
        assert.ok(md instanceof vscode.MarkdownString);
        assert.ok(md.value.includes('DSECT'));
    });

    test('Outline', async () => {
        const outline = services.provideDocumentSymbols(document);

        assert.strictEqual(outline?.length, 1);
        const code = outline[0].children.find(x => x.children.length > 0);
        assert.ok(code);
        assert.ok(code.children.some(x => x.name === '(untitled)'));
    });

    test('Offsets', async () => {
        const offsetsParent = services.provideOffsets(document);

        assert.strictEqual(offsetsParent?.length, 1);
        const offsets = offsetsParent[0].children;

        assert.strictEqual(offsets.length, 3)
        assert.deepStrictEqual(offsets.map(x => ({ name: x.name, line: x.range.start.line })), [
            { name: 'Offset 00000000 (C+00000000)', line: 76 },
            { name: 'Offset 00000000 (C+00000000)', line: 78 },
            { name: 'Offset 00000004 (C+00000004)', line: 79 },
        ]);

    });
});
