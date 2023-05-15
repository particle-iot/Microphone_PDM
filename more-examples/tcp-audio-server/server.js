// Install
// npm install
// 
// Run
// node server.js 

const fs = require('fs');
const path = require('path');
const os = require('os');

const net = require('net');
const wav = require('wav'); // https://github.com/TooTallNate/node-wav

let addresses = [];
{
    const ifaces = os.networkInterfaces();
    
    // http://stackoverflow.com/questions/3653065/get-local-ip-address-in-node-js
    Object.keys(ifaces).forEach(function (ifname) {
        ifaces[ifname].forEach(function (iface) {
            if ('IPv4' !== iface.family || iface.internal !== false) {
                // skip over internal (i.e. 127.0.0.1) and non-ipv4 addresses
                return;
            }
            console.log("found address " + ifname + ": " + iface.address);
            
            addresses.push(iface.address);
        });
    });    
}

const yargs = require('yargs/yargs')
const { hideBin } = require('yargs/helpers')
const argv = yargs(hideBin(process.argv)).argv

// For this simple test, just create wav files in the "out" directory in the directory
// where audioserver.js lives.
var outputDir = path.join(__dirname, "out");  

var dataPort = argv.port || 7123; // this is the port to listen on for data from
console.log('listening on port ' + dataPort);

// If changing the sample frequency in the Particle code, make sure you change this!
var wavOpts = {
	channels: (argv.channels || 1),
	sampleRate: (argv.rate || 16000),
	bitDepth: (argv.bits || 16),
	wireIsUnsigned16: false, // sample app uses unsigned 8 and signed 16, like wav file format, so this is typically false
};
console.log('configuration', wavOpts);

// Output files in the out directory are of the form 00001.wav. lastNum is used 
// to speed up scanning for the next unique file.
var lastNum = 0;

// Create the out directory if it does not exist
try {
	fs.mkdirSync(outputDir);
}
catch(e) {
}

// Start a TCP Server. This is what receives data from the Particle device
// https://gist.github.com/creationix/707146
net.createServer(function (socket) {
	console.log('data connection started from ' + socket.remoteAddress);
	
	// The server sends a 8-bit byte value for each sample. Javascript doesn't really like
	// binary values, so we use setEncoding to read each byte of a data as 2 hex digits instead.
	socket.setEncoding('hex');
	
	var outPath = getUniqueOutputPath();
	
	var writer = new wav.FileWriter(outPath, wavOpts);
	
	socket.on('data', function (data) {
		// We received data on this connection.
		var buf = Buffer.from(data, 'hex');
	
		if (wavOpts.bitDepth == 16 && wavOpts.wireIsUnsigned16) {
			for(var ii = 0; ii < buf.length; ii += 2) {
				var unsigned = buf.readUInt16LE(ii);
				var signed = unsigned - 32768;
				buf.writeInt16LE(signed, ii);
			}
		}
		
		// console.log("got data " + (data.length / 2));
		writer.write(buf);
	});
	socket.on('end', function () {
		console.log('transmission complete, saved to ' + outPath);
		writer.end();
	});
}).listen(dataPort);


function formatName(num) {
	var s = num.toString();
	
	while(s.length < 5) {
		s = '0' + s;
	}
	return s + '.wav';
}

function getUniqueOutputPath() {
	for(var ii = lastNum + 1; ii < 99999; ii++) {
		var outPath = path.join(outputDir, formatName(ii));
		try {
			fs.statSync(outPath);
		}
		catch(e) {
			// File does not exist, use this one
			lastNum = ii;
			return outPath;
		}
	}
	lastNum = 0;
	return "00000.wav";
}

