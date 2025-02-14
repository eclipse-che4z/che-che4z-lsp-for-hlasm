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

import * as vscode from 'vscode';
import { EXTENSION_ID, initialBlanks, languageIdHlasmListing } from './constants';

const ordchar = /[A-Za-z0-9$#@_]/;

// Symbol   Length   Value     Id    R Type Asm  Program   Defn References                     
// A             4 00000000 00000001     A  A                 1   44    45    46    47    48    49    50    51    52    53 
//                                                                54    55    56    57    58    59    60    61    62    63 
//                                                                64    65    66    67    68    69    70    71    72    73 

const listingStart = /^(.?)                                         High Level Assembler Option Summary                   .............   Page    1/;

type RegexSet = {
    objShortCode: RegExp,
    objLongCode: RegExp,
    lineText: RegExp,
    pageBoundary: RegExp,

    ordinaryRefFirstLine: RegExp,
    ordinaryRefAltSecondLine: RegExp,
    ordinaryRefRest: RegExp,
};

const withoutPrefix = {
    objShortCode: /^.{33}( *\d+)\D/,
    objLongCode: /^.{41}( *\d+)\D/,
    lineText: /^(?:(Return Code )|\*\* (ASMA\d\d\d[NIWES] .+)|((?:  |[CDR]-)Loc  Object Code    Addr1 Addr2  Stmt |(?:  |[CDR]-)Loc    Object Code      Addr1    Addr2    Stmt )|(.{111})Page +\d+)/,
    pageBoundary: /^.+(?:(High Level Assembler Option Summary)|(External Symbol Dictionary)|(Relocation Dictionary)|(Ordinary Symbol and Literal Cross Reference)|(Macro and Copy Code Source Summary)|(Dsect Cross Reference)|(Using Map)|(General Purpose Register Cross Reference)|(Diagnostic Cross Reference and Assembler Summary))/,

    ordinaryRefFirstLine: /^(?:([a-zA-Z$#@_][a-zA-Z$#@0-9_]{0,7}) +(\d+) ([0-9A-F]{8}) [0-9A-F]{8} . .... ...  ....... +(\d+) +(\d.+|)|([a-zA-Z$#@_][a-zA-Z$#@0-9_]{8,}))/,
    ordinaryRefAltSecondLine: /^( {9,})(\d+) ([0-9A-F]{8}) [0-9A-F]{8} . .... ...  ....... +(\d+) +(\d.+|)/,
    ordinaryRefRest: /^ {60,}(\d.+)/,
};

const withPrefix = {
    objShortCode: /^..{33}( *\d+)\D/,
    objLongCode: /^..{41}( *\d+)\D/,
    lineText: /^.(?:(Return Code )|\*\* (ASMA\d\d\d[NIWES] .+)|((?:  |[CDR]-)Loc  Object Code    Addr1 Addr2  Stmt |(?:  |[CDR]-)Loc    Object Code      Addr1    Addr2    Stmt )|(.{111})Page +\d+)/,
    pageBoundary: /^.+(?:(High Level Assembler Option Summary)|(External Symbol Dictionary)|(Relocation Dictionary)|(Ordinary Symbol and Literal Cross Reference)|(Macro and Copy Code Source Summary)|(Dsect Cross Reference)|(Using Map)|(General Purpose Register Cross Reference)|(Diagnostic Cross Reference and Assembler Summary))/,

    ordinaryRefFirstLine: /^.(?:([a-zA-Z$#@_][a-zA-Z$#@0-9_]{0,7}) +(\d+) ([0-9A-F]{8}) [0-9A-F]{8} . .... ...  ....... +(\d+) +(\d.+|)|([a-zA-Z$#@_][a-zA-Z$#@0-9_]{8,}))/,
    ordinaryRefAltSecondLine: /^.( {9,})(\d+) ([0-9A-F]{8}) [0-9A-F]{8} . .... ...  ....... +(\d+) +(\d.+|)/,
    ordinaryRefRest: /^. {60,}(\d.+)/,
};

const enum BoudnaryType {
    ReturnStatement,
    Diagnostic,
    ObjectCodeHeader,
    OptionsRef,
    ExternalRef,
    RelocationDict,
    OrdinaryRef,
    MacroRef,
    DsectRef,
    UsingsRef,
    RegistersRef,
    DiagnosticRef,
    OtherBoundary,
};

function testLine(s: string, r: RegexSet): { type: BoudnaryType, capture: string } | undefined {
    const l = r.lineText.exec(s);
    if (!l) return undefined;

    for (let i = 1; i < l.length - 1; ++i) {
        if (l[i])
            return { type: <BoudnaryType>(i - 1), capture: l[i] };
    }

    const m = r.pageBoundary.exec(l[l.length - 1]);
    if (!m) return { type: BoudnaryType.OtherBoundary, capture: l[l.length - 1] };

    for (let i = 1; i < m.length; ++i) {
        if (m[i])
            return { type: <BoudnaryType>(l.length - 2 + i - 1), capture: m[i] };
    }

    return undefined;
}

function asLevel(code: string) {
    if (code.endsWith('N')) return vscode.DiagnosticSeverity.Hint;
    if (code.endsWith('I')) return vscode.DiagnosticSeverity.Information;
    if (code.endsWith('W')) return vscode.DiagnosticSeverity.Warning;
    return vscode.DiagnosticSeverity.Error;
}

type Section = {
    start: number,
    end: number,
};

type Listing = {
    start: number,
    end: number,
    hasPrefix: boolean,
    type?: 'long' | 'short';
    diagnostics: vscode.Diagnostic[],
    statementLines: Map<number, number>,
    symbols: Map<string, Symbol>,
    codeSections: (Section & { title: string, codeStart: number })[],

    options?: Section,
    externals?: Section,
    relocations?: Section,
    ordinary?: Section,
    macro?: Section,
    dsects?: Section,
    usings?: Section,
    registers?: Section,
    summary?: Section,
};

function getCodeRange(l: Listing, line: number) {
    const start = +l.hasPrefix + (l.type === 'long' ? 49 : 40);
    return new vscode.Range(line, start, line, start + 72);
}

function updateSymbols(symbols: Map<string, Symbol>, symbol: Symbol) {
    const name = symbol.name.toUpperCase();
    const s = symbols.get(name);
    if (!s)
        symbols.set(name, symbol);
    else {
        s.defined = [...new Set([...s.defined, ...symbol.defined])];
        s.references = [...new Set([...s.references, ...symbol.references])];
    }
}

class Symbol {
    name: string = '';
    defined: number[] = [];
    references: number[] = [];

    computeReferences(includeDefinition: boolean) {
        return includeDefinition ? [...new Set([...this.references, ...this.defined])] : this.references;
    }
}

function processListing(doc: vscode.TextDocument, start: number, hasPrefix: boolean): { nexti: number, result: Listing } {
    const r: RegexSet = hasPrefix ? withPrefix : withoutPrefix;
    const result: Listing = {
        start,
        end: start,
        hasPrefix,
        diagnostics: [],
        statementLines: new Map<number, number>(),
        symbols: new Map<string, Symbol>(),
        codeSections: [],
    };
    type ListingSections = Exclude<{ [key in keyof Listing]: Listing[key] extends (Section | undefined) ? key : never }[keyof Listing], undefined>;
    const updateCommonSection = <Name extends ListingSections>(name: Name, lineno: number): Listing[Name] => {
        if (!result[name]) {
            result[name] = { start: lineno, end: lineno };
        }
        return result[name];
    };
    const enum States {
        Options,
        Code,
        OrdinaryRefs,
        Refs,
    };

    let symbol: Symbol | undefined;
    let lastSection: { end: number } | undefined = undefined;
    let lastTitle = '';
    let lastTitleLine = start;

    let state = States.Options;
    let i = start;
    main: for (; i < doc.lineCount; ++i) {
        const line = doc.lineAt(i);
        const l = testLine(line.text, r);
        if (l) {
            if (lastSection) {
                lastSection.end = i;
                lastSection = undefined;
            }
            switch (l.type) {
                case BoudnaryType.ReturnStatement:
                    ++i
                    if (result.summary)
                        result.summary.end = i;
                    break main;
                case BoudnaryType.Diagnostic: {
                    const code = l.capture.substring(0, 8);
                    const text = l.capture.substring(9).trim();
                    const d = new vscode.Diagnostic(line.range, text, asLevel(code));
                    d.code = code;
                    result.diagnostics.push(d);
                    break;
                }
                case BoudnaryType.OptionsRef:
                    lastSection = updateCommonSection('options', i);
                    break;
                case BoudnaryType.ObjectCodeHeader: {
                    if (l.capture.length < 45)
                        result.type = 'short';
                    else
                        result.type = 'long';
                    const codeSection = {
                        start: lastTitleLine,
                        end: i + 1,
                        title: lastTitle,
                        codeStart: i + 1,
                    };
                    lastSection = codeSection;
                    result.codeSections.push(codeSection);
                    break;
                }
                case BoudnaryType.ExternalRef:
                    lastSection = updateCommonSection('externals', i);
                    break;
                case BoudnaryType.OrdinaryRef:
                    lastSection = updateCommonSection('ordinary', i);
                    state = States.OrdinaryRefs;
                    ++i; // skip header
                    break;
                case BoudnaryType.MacroRef:
                    lastSection = updateCommonSection('macro', i);
                    state = States.Refs;
                    break;
                case BoudnaryType.DsectRef:
                    lastSection = updateCommonSection('dsects', i);
                    state = States.Refs;
                    break;
                case BoudnaryType.RegistersRef:
                    lastSection = updateCommonSection('registers', i);
                    state = States.Refs;
                    break;
                case BoudnaryType.DiagnosticRef:
                    lastSection = updateCommonSection('summary', i);
                    state = States.Refs;
                    break;
                case BoudnaryType.RelocationDict:
                    lastSection = updateCommonSection('relocations', i);
                    state = States.Refs;
                    break;
                case BoudnaryType.UsingsRef:
                    lastSection = updateCommonSection('usings', i);
                    state = States.Refs;
                    break;
                case BoudnaryType.OtherBoundary:
                    if (state === States.Options)
                        state = States.Code;
                    lastTitle = l.capture.substring(9).trim();
                    lastTitleLine = i;
                    break;
            }
        }
        else if (state === States.Code) {
            const obj = (result.type === 'short' ? r.objShortCode : r.objLongCode).exec(line.text);
            if (obj) {
                result.statementLines.set(parseInt(obj[1]), i);
            }
        }
        else if (state === States.OrdinaryRefs) {
            const ref = r.ordinaryRefFirstLine.exec(line.text);
            let refs = '';
            if (ref) {
                if (symbol)
                    updateSymbols(result.symbols, symbol);

                symbol = new Symbol();
                if (ref[1]) {
                    symbol.name = ref[1];
                    symbol.defined.push(+ref[4]);
                    refs = ref[5];
                }
                else {
                    symbol.name = ref[6];
                }
            }
            else if (symbol) {
                const alt = r.ordinaryRefAltSecondLine.exec(line.text);
                if (alt) {
                    symbol.defined.push(+alt[4]);
                    refs = alt[5];
                }
                else {
                    const cont = r.ordinaryRefRest.exec(line.text);
                    if (cont) {
                        refs = cont[1];
                    }
                }
            }

            if (refs && symbol) {
                for (const m of refs.matchAll(/(\d+)[BDMUX]?/g)) {
                    symbol.references.push(+m[1]);
                }
            }
        }
    }
    if (symbol)
        updateSymbols(result.symbols, symbol);
    if (lastSection) {
        lastSection.end = i;
    }

    result.end = i;
    return { nexti: i, result };
}

function produceListings(doc: vscode.TextDocument): Listing[] {
    const result: Listing[] = []

    for (let i = 0; i < doc.lineCount;) {
        const line = doc.lineAt(i);
        const m = listingStart.exec(line.text);
        if (!m) {
            ++i;
            continue;
        }
        const { nexti, result: listing } = processListing(doc, i, !!m[1]);
        result.push(listing);
        i = nexti;
    }

    return result;
}

function asHover(md: vscode.MarkdownString | undefined): vscode.Hover | undefined {
    return md ? new vscode.Hover(md) : undefined;
}

function isolateSymbolSimple(document: vscode.TextDocument, position: vscode.Position, hasPrefix: boolean) {
    if (position.line >= document.lineCount)
        return undefined;
    const line = document.lineAt(position.line).text;

    let start = position.character;
    let end = position.character;

    while (start > +hasPrefix && ordchar.test(line[start - 1]))
        --start;
    while (end < line.length && ordchar.test(line[end]))
        ++end;

    return line.substring(start, end).toUpperCase();
}

function codeColumns(type: 'short' | 'long', hasPrefix: boolean) {
    if (type === 'short')
        return { left: 40 + +hasPrefix, right: 111 + +hasPrefix };
    else
        return { left: 49 + +hasPrefix, right: 120 + +hasPrefix };
}

function isolateSymbol(l: Listing, document: vscode.TextDocument, position: vscode.Position) {
    const csi = l.codeSections.findIndex(s => s.codeStart <= position.line && position.line < s.end);
    if (!l.type || csi === -1) return isolateSymbolSimple(document, position, l.hasPrefix);
    const cs = l.codeSections[csi];
    const { left, right } = codeColumns(l.type, l.hasPrefix);
    if (position.character < left || position.character >= right) return isolateSymbolSimple(document, position, l.hasPrefix);

    let start = position.character;
    let end = position.character;

    const prevLine = cs.codeStart < position.line ? position.line - 1 : csi === 0 ? -1 : l.codeSections[csi - 1].end - 1;
    const nextLine = position.line + 1 < cs.end ? position.line + 1 : csi === l.codeSections.length - 1 ? -1 : l.codeSections[csi + 1].codeStart;

    const prevText = prevLine >= 0 ? document.lineAt(prevLine).text : '';
    const thisText = document.lineAt(position.line).text;
    const nextText = nextLine >= 0 ? document.lineAt(nextLine).text : '';

    const prevContinued = prevLine >= 0 && /[^ ]/.test(prevText[right]);
    const thisContinued = /[^ ]/.test(thisText[right]);

    const thisOffset = prevContinued ? initialBlanks : 0;

    while (start > left + thisOffset && ordchar.test(thisText[start - 1]))
        --start;
    while (end < right && ordchar.test(thisText[end]))
        ++end;

    let prefix = '';
    let result = thisText.substring(start, end);
    let suffix = '';

    // Handle continuation only one line up and down
    if (prevContinued && start == left + thisOffset) {
        start = right;
        while (start > left + thisOffset && ordchar.test(prevText[start - 1]))
            --start;
        prefix = prevText.substring(start, right);
    }
    if (thisContinued && end == right) {
        end = left + initialBlanks;
        while (end < right && ordchar.test(nextText[end]))
            ++end;
        suffix = nextText.substring(left + initialBlanks, end);
    }

    return (prefix + result + suffix).toUpperCase();
}

function sectionAsSymbol(s: Section, title: string, detail: string = '') {
    const r = new vscode.Range(s.start, 0, s.end, 0);
    return new vscode.DocumentSymbol(title, detail, vscode.SymbolKind.Namespace, r, r);
}

function codeSectionAsSymbol(s: Section & { title: string }) {
    const r = new vscode.Range(s.start, 0, s.end, 0);
    return new vscode.DocumentSymbol(s.title ? s.title : '(untitled)', '', vscode.SymbolKind.Package, r, r);
}

function listingAsSymbol(l: Listing, id: number | undefined) {
    const result = new vscode.DocumentSymbol(
        id ? `Listing ${id}` : 'Listing',
        '',
        vscode.SymbolKind.Module,
        new vscode.Range(l.start, 0, l.end, 0),
        new vscode.Range(l.start, 0, l.end, 0)
    );

    if (l.options) {
        result.children.push(sectionAsSymbol(l.options, 'High Level Assembler Option Summary'));
    }
    if (l.externals) {
        result.children.push(sectionAsSymbol(l.externals, 'External Symbol Dictionary'));
    }
    if (l.codeSections.length > 0) {
        const code = sectionAsSymbol({ start: l.codeSections[0].start, end: l.codeSections[l.codeSections.length - 1].end }, 'Object Code');
        result.children.push(code);
        code.children = l.codeSections.reduce<typeof l.codeSections>((acc, c) => {
            const last = acc[acc.length - 1];
            if (last?.title === c.title) {
                last.start = Math.min(last.start, c.start);
                last.end = Math.max(last.end, c.end);
            }
            else {
                acc.push(c);
            }
            return acc;
        }, []).map(x => codeSectionAsSymbol(x));
    }
    if (l.relocations) {
        result.children.push(sectionAsSymbol(l.relocations, 'Relocation Dictionary'));
    }
    if (l.ordinary) {
        result.children.push(sectionAsSymbol(l.ordinary, 'Ordinary Symbol and Literal Cross Reference'));
    }
    if (l.macro) {
        result.children.push(sectionAsSymbol(l.macro, 'Macro and Copy Code Source Summary'));
    }
    if (l.dsects) {
        result.children.push(sectionAsSymbol(l.dsects, 'Dsect Cross Reference'));
    }
    if (l.usings) {
        result.children.push(sectionAsSymbol(l.usings, 'Using Map'));
    }
    if (l.registers) {
        result.children.push(sectionAsSymbol(l.registers, 'General Purpose Register Cross Reference'));
    }
    if (l.summary) {
        result.children.push(sectionAsSymbol(l.summary, 'Diagnostic Cross Reference and Assembler Summary'));
    }

    return result;
}

function findBestFitLine(m: Map<number, number>, s: number): number | undefined {
    let result;

    for (const k of m.keys()) {
        if (k > s) continue;
        result = result ? Math.max(k, result) : k;
    }

    if (result)
        return m.get(result);
    else
        return undefined;
}

function compareNumbers(l: number, r: number) {
    return l - r;
}

export function createListingServices(diagCollection?: vscode.DiagnosticCollection) {
    const listings = new Map<string, Listing[]>();

    function handleListingContent(doc: vscode.TextDocument) {
        if (doc.languageId !== languageIdHlasmListing) return;
        const initVersion = doc.version;
        const data = produceListings(doc);
        if (initVersion !== doc.version || doc.languageId !== languageIdHlasmListing) return;
        listings.set(doc.uri.toString(), data);
        diagCollection?.set(doc.uri, data.flatMap(x => x.diagnostics));

        return data;
    }

    function releaseListingContent(doc: vscode.TextDocument) {
        const uri = doc.uri.toString();
        listings.delete(uri);
        diagCollection?.delete(doc.uri);
    }

    function symbolFunction<R, Args extends any[]>(f: (symbol: Symbol, l: Listing, document: vscode.TextDocument, ...args: Args) => R) {
        return (document: vscode.TextDocument, position: vscode.Position, ...args: Args): R | undefined => {
            const l = (listings.get(document.uri.toString()) ?? handleListingContent(document) ?? []).find(x => x.start <= position.line && position.line < x.end);
            if (!l) return undefined;
            const symName = isolateSymbol(l, document, position);
            if (!symName) return undefined;
            const symbol = l.symbols.get(symName);
            if (!symbol) return undefined;
            return f(symbol, l, document, ...args);
        }
    }

    return {
        handleListingContent,
        releaseListingContent,
        provideDefinition: symbolFunction((symbol, l, document) => symbol.defined
            .map(x => l.statementLines.get(x) || findBestFitLine(l.statementLines, x))
            .filter((x): x is number => typeof x === 'number')
            .sort(compareNumbers)
            .map(x => new vscode.Location(document.uri, getCodeRange(l, x))))
        ,
        provideReferences: symbolFunction((symbol, l, document, context: vscode.ReferenceContext) => symbol.computeReferences(context.includeDeclaration)
            .map(x => l.statementLines.get(x))
            .filter((x): x is number => typeof x === 'number')
            .sort(compareNumbers)
            .map(x => new vscode.Location(document.uri, getCodeRange(l, x)))
        ),
        provideHover: symbolFunction((symbol, l, document) => asHover(symbol.defined
            .map(x => l.statementLines.get(x))
            .filter((x): x is number => typeof x === 'number')
            .sort(compareNumbers)
            .map(x => document.lineAt(x).text)
            .reduce((acc, cur) => { return acc.appendCodeblock(cur, languageIdHlasmListing); }, new vscode.MarkdownString()))
        ),
        provideDocumentSymbols: (document: vscode.TextDocument) =>
            (listings.get(document.uri.toString()) ?? handleListingContent(document))?.map((l, id, ar) => listingAsSymbol(l, ar.length > 1 ? id + 1 : undefined))
        ,
    };
}

export function registerListingServices(context: vscode.ExtensionContext) {
    const diagCollection = vscode.languages.createDiagnosticCollection(EXTENSION_ID + '.listings');
    context.subscriptions.push(diagCollection);

    const services = createListingServices(diagCollection);

    vscode.workspace.onDidOpenTextDocument(services.handleListingContent, undefined, context.subscriptions);
    vscode.workspace.onDidChangeTextDocument(({ document: doc }) => services.handleListingContent(doc), undefined, context.subscriptions);
    vscode.workspace.onDidCloseTextDocument(services.releaseListingContent, undefined, context.subscriptions);

    Promise.allSettled(vscode.workspace.textDocuments.filter(x => x.languageId === languageIdHlasmListing).map(x => services.handleListingContent(x))).catch(() => { });

    context.subscriptions.push(vscode.languages.registerDefinitionProvider(languageIdHlasmListing, services));
    context.subscriptions.push(vscode.languages.registerReferenceProvider(languageIdHlasmListing, services));
    context.subscriptions.push(vscode.languages.registerHoverProvider(languageIdHlasmListing, services));
    context.subscriptions.push(vscode.languages.registerDocumentSymbolProvider(languageIdHlasmListing, services));
}
