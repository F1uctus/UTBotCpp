// Drive the server the way the extension does, over gRPC, from the extension's
// own generated stubs.
//
// Everything else in CI exercises the server through its command line. That
// leaves the half a user actually touches untested: the editor never calls
// utbot.exe, it opens a channel and sends ConfigureProject and
// GenerateProjectTests, and the answers come back as a stream. A break in that
// path -- a field the server reads differently, a stream that never completes,
// a path spelled for the wrong platform -- would not show up in any smoke test
// that runs a subcommand.
//
// Deliberately not a VS Code integration test: those need a downloaded editor
// and a display, and would test the editor's plumbing rather than ours. The
// stubs used here are the ones compiled into the extension, so what is checked
// is the contract between the two halves.

const path = require('path');
const fs = require('fs');
// Spelled the way package.json installs it: the dependency is an alias,
// "grpc": "npm:@grpc/grpc-js", so that is the name on disk -- and it is the
// name the extension's own sources import it by.
const grpc = require('grpc');

const [, , projectPath, outDir, portArg] = process.argv;
if (!projectPath || !outDir) {
    console.error('usage: serverHandshake.js <projectPath> <testsRelDir> [port]');
    process.exit(2);
}
const port = portArg || '2121';

const stubs = path.resolve(__dirname, '../../proto-ts');
const services = require(path.join(stubs, 'testgen_grpc_pb'));
const messages = require(path.join(stubs, 'testgen_pb'));

const client = new services.TestsGenServiceClient(
    `127.0.0.1:${port}`, grpc.credentials.createInsecure());

function fail(what, err) {
    console.error(`FAILED: ${what}: ${err && err.message ? err.message : err}`);
    process.exit(1);
}

const deadline = (seconds) => new Date(Date.now() + seconds * 1000);

function unary(name, request, seconds) {
    return new Promise((resolve, reject) => {
        client[name](request, { deadline: deadline(seconds) }, (err, response) =>
            err ? reject(err) : resolve(response));
    });
}

// The generation and configuration calls answer with a stream: progress first,
// then whatever they produced. Waiting for 'end' is what tells us the server
// finished rather than merely started.
function streamed(name, request, seconds, onData) {
    return new Promise((resolve, reject) => {
        const call = client[name](request, { deadline: deadline(seconds) });
        const seen = [];
        call.on('data', (response) => { seen.push(response); if (onData) onData(response); });
        call.on('error', reject);
        call.on('end', () => resolve(seen));
    });
}

function projectContext() {
    const context = new messages.ProjectContext();
    context.setProjectname(path.basename(projectPath));
    context.setProjectpath(projectPath);
    // What the extension sends: where the project is on the machine the editor
    // runs on. Same place here, which is the local scenario.
    context.setClientprojectpath(projectPath);
    context.setTestdirrelpath(outDir);
    context.setReportdirrelpath('utbot_report');
    context.setBuilddirrelpath('build');
    context.setItfrelpath('');
    return context;
}

function settingsContext() {
    const settings = new messages.SettingsContext();
    settings.setGenerateforstaticfunctions(true);
    settings.setVerbose(false);
    settings.setTimeoutperfunction(15);
    settings.setTimeoutpertest(0);
    settings.setUsedeterministicsearcher(false);
    settings.setUsestubs(false);
    settings.setDifferentvariablesofthesametype(false);
    settings.setSkipobjectwithoutsource(false);
    return settings;
}

async function main() {
    const version = new messages.VersionInfo();
    version.setVersion('e2e');
    const handshake = await unary('handshake', version, 30)
        .catch((err) => fail('handshake', err));
    console.log(`handshake: server reports ${handshake.getVersion()}`);

    const registration = new messages.RegisterClientRequest();
    registration.setClientid('e2e');
    await unary('registerClient', registration, 30)
        .catch((err) => fail('registerClient', err));
    console.log('registered');

    const configure = new messages.ProjectConfigRequest();
    configure.setProjectcontext(projectContext());
    configure.setConfigmode(messages.ConfigMode.ALL);
    const configured = await streamed('configureProject', configure, 900)
        .catch((err) => fail('configureProject', err));
    const statuses = configured.map((r) => r.getType());
    console.log(`configureProject: ${configured.length} responses, statuses ${statuses.join(',')}`);
    // IS_OK is 0; anything else names a way the import did not happen.
    const bad = statuses.filter((s) => s !== messages.ProjectConfigStatus.IS_OK);
    if (bad.length) {
        const messagesOut = configured.map((r) => r.getMessage()).filter(Boolean);
        fail('configureProject', `statuses ${bad.join(',')}: ${messagesOut.join(' | ')}`);
    }
    for (const database of ['compile_commands.json', 'link_commands.json']) {
        const where = path.join(projectPath, 'build', database);
        if (!fs.existsSync(where)) fail('configureProject', `no ${database}`);
    }
    console.log('configureProject: both build databases written');

    const generate = new messages.ProjectRequest();
    generate.setProjectcontext(projectContext());
    generate.setSettingscontext(settingsContext());
    generate.setSourcepathsList([path.join(projectPath, 'src')]);
    // False, as the extension sends it when the server shares this filesystem:
    // the server writes the files itself and the client is told where.
    generate.setSynchronizecode(false);
    const responses = await streamed('generateProjectTests', generate, 1800)
        .catch((err) => fail('generateProjectTests', err));

    const written = [];
    for (const response of responses) {
        for (const source of response.getTestsourcesList()) {
            written.push(source.getFilepath());
        }
    }
    console.log(`generateProjectTests: ${responses.length} responses, ${written.length} sources`);
    if (!written.length) fail('generateProjectTests', 'the server named no test source');

    const tests = written.filter((p) => p.endsWith('_test.cpp'));
    if (!tests.length) fail('generateProjectTests', `no _test.cpp among ${written.join(', ')}`);
    for (const test of tests) {
        if (!fs.existsSync(test)) fail('generateProjectTests', `${test} was named but not written`);
    }
    console.log(`generateProjectTests: ${tests.length} test files exist on disk`);
    console.log('the extension\'s half of the protocol works end to end');
    client.close();
}

main().catch((err) => fail('unexpected', err));
