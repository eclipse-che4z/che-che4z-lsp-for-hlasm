const fs = require('fs')
const path = require('path')

module.exports = {
    recursiveCopySync: function(origin, dest) {
        if (fs.existsSync(origin)) {
            if (fs.statSync(origin).isDirectory()) {
                fs.mkdirSync(dest);
                fs.readdirSync(origin).forEach(file => 
                    this.recursiveCopySync(path.join(origin, file), path.join(dest, file)));
            }
            else {
                fs.copyFileSync(origin, dest);
            }
        }
        else {
            console.log(origin)
        }
    },

    recursiveRemoveSync: function(dest) {
        if (fs.existsSync(dest)) {
            fs.readdirSync(dest).forEach(file => {
            const currPath = path.join(dest, file);
            if (fs.statSync(currPath).isDirectory()) {
                this.recursiveRemoveSync(currPath);
            } 
            else { 
                fs.unlinkSync(currPath);
            }
            });
            fs.rmdirSync(dest);
        }
    }
}