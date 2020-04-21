const methods = require('./syncFolderMethods')
const path = require('path');

methods.recursiveRemoveSync(path.join(__dirname, '../../lib/test/workspace/'));
methods.recursiveCopySync(path.join(__dirname, '../../src/test/workspace/'),path.join(__dirname, '../../lib/test/workspace/'))

console.log('Test workspace ready')