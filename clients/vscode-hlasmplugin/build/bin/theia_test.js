const process = require('process');
const { execFile, spawn } = require('child_process');
const puppeteer = require('puppeteer');

const hlasmpluginDir = process.argv[2];
const theiaDir = process.argv[3];

async function main() {
    // prepare plugin for theia
    process.env.THEIA_DEFAULT_PLUGINS='local-dir:'+hlasmpluginDir+'/build/bin';

    // run integration tests as plugin for theia
    const child = spawn('node', [
        theiaDir+'/examples/browser/src-gen/backend/main.js',
        hlasmpluginDir+'/lib/test/workspace/',
        '--extensionTestsPath='+hlasmpluginDir+'/lib/test/suite', 
        '--hostname', '0.0.0.0',
        '--port','3000' 
    ]);

    // give theia 5 seconds to start, then connect
    setTimeout(async function() {
        const browser = await puppeteer.launch({headless: true});
        const page = await browser.newPage();
        try {
            await page.goto('http://localhost:3000');
        }
        catch (err) {
            console.log(err);
        }
    },5000);

    // check for the results
    child.stdout.on('data', function(data) {
        console.log(data.toString());
        if (data.toString().includes('8 passing')) {
            process.exit();
        }
    })

    child.stderr.on('data', function(data) {
        console.log(data.toString());
        if(data.toString().includes('tests failed')) {
            process.exit(2);
        }
    })
}

main();