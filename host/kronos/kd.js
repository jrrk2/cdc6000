#!/usr/bin/env node
// Daemon: starts DtCyber/KRONOS 2.1 and serves job submissions on a unix socket.
//   run:  cd <KRONOS2.1 dir>; DISPLAY=:99 setsid sh -c 'tail -f /dev/null | node /path/kd.js' &
const path = require('path'), net = require('net'), fs = require('fs');
const K = path.resolve(__dirname, '../../src/DtCyber/KRONOS2.1');
process.chdir(K);
const DtCyber = require(path.join(K, '../automation/DtCyber'));
const SOCK = process.env.KD_SOCK || '/tmp/claude-1000/kd.sock';
const dtc = new DtCyber();
let busy = Promise.resolve();

dtc.start({ detached: true, stdio: [0, 'ignore', 2], unref: false })
.then(() => dtc.say('DtCyber started'))
.then(() => dtc.sleep(2000))
.then(() => dtc.attachPrinter('LP5xx_C12_E6'))
.then(() => dtc.expect([{ re: /QUEUE FILE UTILITY COMPLETE/ }], 'printer'))
.then(() => dtc.connect())
.then(() => dtc.expect([{ re: /Operator> $/ }]))
.then(() => {
  dtc.say('KRONOS ready; serving ' + SOCK);
  try { fs.unlinkSync(SOCK); } catch (e) {}
  net.createServer(c => {
    let buf = '';
    c.on('data', d => {
      buf += d;
      let i;
      while ((i = buf.indexOf('\n')) >= 0) {
        const line = buf.slice(0, i); buf = buf.slice(i + 1);
        let req; try { req = JSON.parse(line); } catch (e) { c.write('bad request\n'); continue; }
        busy = busy.then(() => handle(req, c)).catch(e => c.write('ERROR ' + e.message + '\n'))
                   .then(() => c.end());
      }
    });
    c.on('error', () => {});
  }).listen(SOCK);
})
.catch(err => { console.log(err); process.exit(1); });

function handle(req, c) {
  if (req.cmd === 'job') {
    return dtc.runJob(12, 4, req.deck, data => c.write(data.toString('latin1')))
      .catch(e => c.write('\n[' + e.message + ']\n'));
  }
  if (req.cmd === 'shutdown') {
    c.write('shutting down\n');
    return dtc.dsd('[UNLOCK.').then(() => dtc.dsd('[BLITZ.')).then(() => dtc.sleep(5000))
      .then(() => dtc.dsd('CHECK#2000#')).then(() => dtc.sleep(2000)).then(() => dtc.dsd('STEP.'))
      .then(() => dtc.sleep(1000)).then(() => dtc.send('shutdown'))
      .then(() => dtc.expect([{ re: /Goodbye for now/ }])).then(() => process.exit(0));
  }
  c.write('unknown command\n');
}
