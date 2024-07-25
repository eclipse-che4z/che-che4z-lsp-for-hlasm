/*
 * Copyright (c) 2024 Broadcom.
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

if (typeof importScripts == "function") {
    const tmpQueue = [];
    self.onmessage = (event) => {
        self.onmessage = (event) => { tmpQueue.push(event); };

        const { extensionUri, arguments } = event.data;

        Module({
            tmpQueue,
            worker: self,
            arguments,
            mainScriptUrlOrBlob: self.location.href,
            locateFile(path) {
                if (typeof path !== 'string') return path;
                if (path.endsWith(".wasm")) {
                    return extensionUri + 'bin/wasm/hlasm_language_server.wasm';
                }
                return path;
            },
        });
    }
}
else if (require.main === module) {
    Module();
}
