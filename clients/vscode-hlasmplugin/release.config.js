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

const { execSync } = require('child_process');
const { pathToFileURL } = require('node:url');

function getReleaseNotePlugin() {
    return [
        '@semantic-release/release-notes-generator',
        {
            'preset': 'conventionalcommits',
            'presetConfig': {
                'types': [
                    {
                        'type': 'feat',
                        'section': 'Features',
                        'hidden': false
                    },
                    {
                        'type': 'fix',
                        'section': 'Fixes',
                        'hidden': false
                    },
                    {
                        'type': 'docs',
                        'section': 'Other changes',
                        'hidden': false
                    },
                    {
                        'type': 'style',
                        'section': 'Other changes',
                        'hidden': false
                    },
                    {
                        'type': 'refactor',
                        'section': 'Other changes',
                        'hidden': false
                    },
                    {
                        'type': 'perf',
                        'section': 'Fixes',
                        'hidden': false
                    },
                    {
                        'type': 'test',
                        'section': 'Other changes',
                        'hidden': false
                    },
                    {
                        'type': 'ci',
                        'section': 'Other changes',
                        'hidden': false
                    },
                    {
                        'type': 'chore',
                        'hidden': true
                    }
                ]
            }
        }
    ];
}

function getCommitAnalyzerPlugin() {
    return [
        '@semantic-release/commit-analyzer',
        {
            'preset': 'conventionalcommits'
        }
    ];
}

function getNextReleasePlugin() {
    return [
        '@semantic-release/exec',
        {
            generateNotesCmd: `sh prepare_release.sh \${nextRelease.version} \${branch.name} "\${nextRelease.notes.replaceAll('"','\\"')}" \${lastRelease.version} \${Date.now()}`
        }
    ];
}

function getGitHubPublisherPlugin() {
    return [
        '@semantic-release/github',
        {
            'assets': [
                {
                    'path': 'hlasm-language-support.vsix',
                    'label': 'hlasm-language-support-${nextRelease.version}.vsix',
                    'name': 'hlasm-language-support-${nextRelease.version}.vsix'
                },
                {
                    'path': 'hlasm-language-support-web.vsix',
                    'label': 'hlasm-language-support-web-${nextRelease.version}.vsix',
                    'name': 'hlasm-language-support-web-${nextRelease.version}.vsix'
                }
            ]
        }
    ];
}

function getDefaultBranchConfig() {
    return [
        'master',
        {
            name: 'release-next',
            prerelease: 'beta',
        }
    ];
}

function getDefaultConfig() {
    return {
        branches: getDefaultBranchConfig(),
        ci: true,
        tagFormat: '${version}',
        plugins: [
            getCommitAnalyzerPlugin(),
            getReleaseNotePlugin(),
            getNextReleasePlugin(),
            getGitHubPublisherPlugin(),
        ],
    };
}

function getAlphaConfig() {
    return {
        repositoryUrl: pathToFileURL(execSync('git rev-parse --show-toplevel').toString().trim()).toString(),
        branches: [
            'master',
            {
                name: execSync('git rev-parse --abbrev-ref HEAD').toString().trim(),
                prerelease: 'alpha',
            },
            {
                name: 'refs/pull/.+',
                prerelease: 'alpha',
            },
        ],
        ci: false,
        tagFormat: '${version}',
        plugins: [
            getCommitAnalyzerPlugin(),
            getReleaseNotePlugin(),
            getNextReleasePlugin(),
        ],
    };
}

function isAlphaBuild() {
    return process.env.HLASM_ALPHA_BUILD === '1';
}

module.exports = isAlphaBuild() ? getAlphaConfig() : getDefaultConfig();

