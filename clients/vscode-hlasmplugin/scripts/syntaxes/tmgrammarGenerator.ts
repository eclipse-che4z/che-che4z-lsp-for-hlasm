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

import * as fs from 'fs'

const syntaxesDir = "syntaxes";

const grammar_details_template = './scripts/syntaxes/hlasm_tmgrammar_template.txt';
const grammar_base_template = './scripts/syntaxes/hlasm_base_template.txt';

const code_block_listing_begin = '.{2}Loc  Object Code    Addr1 Addr2  Stmt   Source Statement.*';
const code_block_listing_long_begin = '.{2}Loc    Object Code      Addr1    Addr2    Stmt  Source Statement.*';
const code_block_listing_annotation_length = '.{40}';
const code_block_listing_long_annotation_length = '.{49}';

const emptyRule = 'emptyRule'
const asmaRule = 'asma'
const pageAnnotationRule = 'pageAnnotation'
const ignoredSequenceNumbers = 'ignoredSequenceNumbers'
const listingSequenceNumbers = 'listingSequenceNumbers'

interface GrammarFile {
  file: string;
  grammarName: string;
  scope: string;
}

interface GrammarDetails extends GrammarFile {
  entryPattern: string;
  codeBlockBegin: string;
  beginLineSkipRule: string;
  listingOffset: string;
  asmaRule: string;
  pageAnnotationRule: string;
  ignoredHandling: string;
}

const hlasmGeneralGrammar: GrammarDetails = {
  file: './syntaxes/hlasmGeneral.tmLanguage.json',
  grammarName: 'HLASM General',
  scope: 'hlasmGeneral',
  entryPattern: 'hlasm_syntax',
  codeBlockBegin: '',
  beginLineSkipRule: '',
  listingOffset: '',
  asmaRule: emptyRule,
  pageAnnotationRule: emptyRule,
  ignoredHandling: ignoredSequenceNumbers,
}

const hlasmListingGeneralGrammar: GrammarDetails = {
  file: './syntaxes/hlasmListingGeneral.tmLanguage.json',
  grammarName: 'HLASM Listing General',
  scope: 'hlasmListingGeneral',
  entryPattern: 'code_block',
  codeBlockBegin: code_block_listing_begin,
  beginLineSkipRule: code_block_listing_annotation_length,
  listingOffset: '',
  asmaRule: asmaRule,
  pageAnnotationRule: pageAnnotationRule,
  ignoredHandling: listingSequenceNumbers,
}

const hlasmListingGeneralLongGrammar: GrammarDetails = {
  file: './syntaxes/hlasmListingGeneralLong.tmLanguage.json',
  grammarName: 'HLASM Listing General Long',
  scope: 'hlasmListingGeneralLong',
  entryPattern: 'code_block',
  codeBlockBegin: code_block_listing_long_begin,
  beginLineSkipRule: code_block_listing_long_annotation_length,
  listingOffset: '',
  asmaRule: asmaRule,
  pageAnnotationRule: pageAnnotationRule,
  ignoredHandling: listingSequenceNumbers,
}

const hlasmListingEndevorGrammar: GrammarDetails = {
  file: './syntaxes/hlasmListingEndevor.tmLanguage.json',
  grammarName: 'HLASM Listing Endevor',
  scope: 'hlasmListingEndevor',
  entryPattern: 'code_block',
  codeBlockBegin: code_block_listing_begin,
  beginLineSkipRule: code_block_listing_annotation_length,
  listingOffset: '.',
  asmaRule: asmaRule,
  pageAnnotationRule: pageAnnotationRule,
  ignoredHandling: listingSequenceNumbers,
}

const hlasmListingEndevorLongGrammar: GrammarDetails = {
  file: './syntaxes/hlasmListingEndevorLong.tmLanguage.json',
  grammarName: 'HLASM Listing Endevor Long',
  scope: 'hlasmListingEndevorLong',
  entryPattern: 'code_block',
  codeBlockBegin: code_block_listing_long_begin,
  beginLineSkipRule: code_block_listing_long_annotation_length,
  listingOffset: '.',
  asmaRule: asmaRule,
  pageAnnotationRule: pageAnnotationRule,
  ignoredHandling: listingSequenceNumbers,
}

interface GrammarBase extends GrammarFile {
  includedGrammars: GrammarDetails[];
}

const hlasmBase: GrammarBase = {
  file: './syntaxes/hlasm.tmLanguage.json',
  grammarName: 'HLASM',
  scope: 'hlasm',
  includedGrammars: [hlasmGeneralGrammar]
}

const hlasmListingBase: GrammarBase = {
  file: './syntaxes/hlasmListing.tmLanguage.json',
  grammarName: 'HLASM Listing',
  scope: 'hlasmListing',
  includedGrammars: [
    hlasmListingGeneralGrammar,
    hlasmListingGeneralLongGrammar,
    hlasmListingEndevorGrammar,
    hlasmListingEndevorLongGrammar
  ]
}

function generateGrammarsDetails(props: GrammarDetails) {
  const listingDetails = props.listingOffset + props.beginLineSkipRule;

  fs.readFile(grammar_details_template, 'utf8', (err: any, data: string) => {
    if (err) {
      return console.log(err);
    }

    let result = data.replaceAll('${grammarName}$', props.grammarName);
    result = result.replaceAll('${scope}$', props.scope);
    result = result.replaceAll('${entryPattern}$', props.entryPattern);
    result = result.replaceAll('${asma}$', props.asmaRule);
    result = result.replaceAll('${pageAnnotation}$', props.pageAnnotationRule);
    result = result.replaceAll('${codeBlockBegin}$', props.codeBlockBegin);
    result = result.replaceAll('${listingOffset}$', props.listingOffset);
    result = result.replaceAll('${ignoredHandling}$', props.ignoredHandling);
    result = result.replaceAll('${listingDetails}$', listingDetails);

    if (props.codeBlockBegin.length === 0)
      result = result.replaceAll('${noPrecedingCodeBlock}$', '');
    else
      result = result.replaceAll('${noPrecedingCodeBlock}$', '(?<!^' + props.listingOffset + props.codeBlockBegin + ')');

    fs.writeFile(props.file, result, 'utf8', (err: any) => {
      if (err) return console.log(err);
    });
  });
}

function generateGrammarBase(props: GrammarBase) {
  fs.readFile(grammar_base_template, 'utf8', (err: any, data: string) => {
    let includeRule = 'include: #{}';

    if (err) {
      return console.log(err);
    }

    if (props.includedGrammars.length !== 0) {
      includeRule = '{"include": "source.' + props.includedGrammars[0].scope + '"}'

      for (let i = 1; i < props.includedGrammars.length; ++i)
        includeRule += ',{"include": "source.' + props.includedGrammars[i].scope + '"}'
    }

    let result = data.replaceAll('${grammarName}$', props.grammarName);
    result = result.replaceAll('${scope}$', props.scope);
    result = result.replaceAll('${patterns}$', includeRule);

    fs.writeFile(props.file, result, 'utf8', (err: any) => {
      if (err) return console.log(err);
    });
  });
}

if (!fs.existsSync(syntaxesDir)) {
  fs.mkdirSync(syntaxesDir);
}

generateGrammarsDetails(hlasmGeneralGrammar);
generateGrammarsDetails(hlasmListingGeneralGrammar);
generateGrammarsDetails(hlasmListingGeneralLongGrammar);
generateGrammarsDetails(hlasmListingEndevorGrammar);
generateGrammarsDetails(hlasmListingEndevorLongGrammar);

generateGrammarBase(hlasmBase);
generateGrammarBase(hlasmListingBase);
