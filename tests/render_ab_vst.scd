(
var root = "DRAGONFLY_DF_ROOT".getenv;
var outDir;
var vstDir = "DRAGONFLY_VST3_DIR".getenv ? "/Library/Audio/Plug-Ins/VST3";
var pluginPath;

var duration = 4.0;
var sampleRate = 48000;
var blockSize = 64;
var sourceBus;
var cases;

var norm = { |value, lo, hi| ((value - lo) / (hi - lo)).clip(0, 1) };

var renderNative;
var renderVST;
var finish;

root.isNil.if {
	root = PathName(thisProcess.nowExecutingPath).pathOnly +/+ "..";
};

outDir = root +/+ "build-3.14.1/ab";
pluginPath = [
	root +/+ "build-3.14.1/stage/DragonflyReverbDF",
	root +/+ "tests/vstplugin-scsynth-only",
	"/Applications/SuperCollider.app/Contents/Resources/plugins"
].join($:);

"mkdir -p %".format(outDir.escapeChar($ )).unixCmd;

s.options.numInputBusChannels = 0;
s.options.numOutputBusChannels = 2;
s.options.sampleRate = sampleRate;
s.options.blockSize = blockSize;
s.options.memSize = 262144;
s.options.ugenPluginsPath = pluginPath;

cases = [
	(
		name: "early",
		key: "Dragonfly Early Reflections.vst3",
		native: { |src|
			EarlyReflectionsDF.ar(src[0], src[1], dry: 40, wet: 60, program: 2, size: 20, width: 100, lowCut: 50, highCut: 10000)
		},
		params: [
			0, norm.(40, 0, 100),
			1, norm.(60, 0, 100),
			2, norm.(2, 0, 7),
			3, norm.(20, 10, 60),
			4, norm.(100, 50, 150),
			5, norm.(50, 0, 200),
			6, norm.(10000, 1000, 16000)
		]
	),
	(
		name: "hall",
		key: "Dragonfly Hall Reverb.vst3",
		native: { |src|
			HallDF.ar(src[0], src[1], dry: 35, early: 25, late: 45, size: 24, width: 100, predelay: 8, diffuse: 85, lowCut: 4, lowXover: 500, lowMult: 1.3, highCut: 7600, highXover: 5500, highMult: 0.5, spin: 3.3, wander: 15, decay: 1.3, earlySend: 20, modulation: 15)
		},
		params: [
			0, norm.(35, 0, 100),
			1, norm.(25, 0, 100),
			2, norm.(45, 0, 100),
			3, norm.(24, 10, 60),
			4, norm.(100, 50, 150),
			5, norm.(8, 0, 100),
			6, norm.(85, 0, 100),
			7, norm.(4, 0, 200),
			8, norm.(500, 200, 1200),
			9, norm.(1.3, 0.5, 2.5),
			10, norm.(7600, 1000, 16000),
			11, norm.(5500, 1000, 16000),
			12, norm.(0.5, 0.2, 1.2),
			13, norm.(3.3, 0, 10),
			14, norm.(15, 0, 40),
			15, norm.(1.3, 0.1, 10),
			16, norm.(20, 0, 100),
			17, norm.(15, 0, 100)
		]
	),
	(
		name: "plate",
		key: "Dragonfly Plate Reverb.vst3",
		native: { |src|
			PlateDF.ar(src[0], src[1], dry: 35, wet: 55, algorithm: 1, width: 100, predelay: 5, decay: 0.8, lowCut: 100, highCut: 12000, damp: 7000)
		},
		params: [
			0, norm.(35, 0, 100),
			1, norm.(55, 0, 100),
			2, norm.(1, 0, 2),
			3, norm.(100, 50, 150),
			4, norm.(5, 0, 100),
			5, norm.(0.8, 0.1, 10),
			6, norm.(100, 0, 200),
			7, norm.(12000, 1000, 16000),
			8, norm.(7000, 1000, 16000)
		]
	),
	(
		name: "room",
		key: "Dragonfly Room Reverb.vst3",
		native: { |src|
			RoomDF.ar(src[0], src[1], dry: 35, early: 20, earlySend: 20, late: 45, size: 12, width: 100, predelay: 6, decay: 0.5, diffuse: 70, spin: 0.8, wander: 40, inHighCut: 16000, earlyDamp: 10000, lateDamp: 9400, boost: 50, boostLPF: 600, inLowCut: 4)
		},
		params: [
			0, norm.(35, 0, 100),
			1, norm.(20, 0, 100),
			2, norm.(20, 0, 100),
			3, norm.(45, 0, 100),
			4, norm.(12, 8, 32),
			5, norm.(100, 50, 150),
			6, norm.(6, 0, 100),
			7, norm.(0.5, 0.1, 10),
			8, norm.(70, 0, 100),
			9, norm.(0.8, 0, 5),
			10, norm.(40, 0, 100),
			11, norm.(16000, 1000, 16000),
			12, norm.(10000, 1000, 16000),
			13, norm.(9400, 1000, 16000),
			14, norm.(50, 0, 100),
			15, norm.(600, 50, 1050),
			16, norm.(4, 0, 200)
		]
	)
];

SynthDef(\dragonflyABSource, { |out = 0, amp = 0.2, dur = 4|
	var click = Decay2.ar(Impulse.ar(1.5), 0.001, 0.04) * SinOsc.ar([220, 330]).sum;
	var tone = SinOsc.ar(XLine.kr(110, 880, dur), 0, 0.25);
	var env = EnvGen.kr(Env.linen(0.01, dur - 0.02, 0.01), doneAction: 2);
	Out.ar(out, [click + tone, DelayC.ar(click - tone, 0.02, 0.007)] * amp * env);
}).add;

SynthDef(\dragonflyABVST, { |inbus = 0, outbus = 0|
	var src = In.ar(inbus, 2);
	Out.ar(outbus, VSTPlugin.ar(src, 2, id: \vst));
}).add;

cases.do { |case|
	SynthDef(("dragonflyABNative_" ++ case[\name]).asSymbol, { |inbus = 0, outbus = 0|
		var src = In.ar(inbus, 2);
		Out.ar(outbus, case[\native].(src));
	}).add;
};

SynthDef(\dragonflyABDiskOut, { |bufnum = 0, inbus = 0|
	DiskOut.ar(bufnum, In.ar(inbus, 2));
}).add;

s.waitForBoot {
	sourceBus = Bus.audio(s, 2);
	s.sync;
	VSTPlugin.search(s, vstDir, options: (save: false, parallel: false, timeout: 20), verbose: false, action: {
		Routine({
			var recBuf;
			cases.do { |case|
				renderNative.(case);
				renderVST.(case);
			};
			finish.();
		}).play(AppClock);
	});
};

renderNative = { |case|
	var path = outDir +/+ (case[\name] ++ "_native.aiff");
	var recBuf = Buffer.alloc(s, 65536, 2);
	var rec;
	var source;
	var effect;
	s.sync;
	recBuf.write(path, "AIFF", "int24", 0, 0, true);
	s.sync;
	source = Synth(\dragonflyABSource, [\out, sourceBus, \dur, duration]);
	effect = Synth.after(source, ("dragonflyABNative_" ++ case[\name]).asSymbol, [\inbus, sourceBus, \outbus, 0]);
	rec = Synth.after(effect, \dragonflyABDiskOut, [\bufnum, recBuf.bufnum, \inbus, 0]);
	duration.wait;
	rec.free;
	effect.free;
	0.1.wait;
	recBuf.close;
	recBuf.free;
	s.sync;
	("AB_NATIVE " ++ path).postln;
};

renderVST = { |case|
	var path = outDir +/+ (case[\name] ++ "_vst.aiff");
	var recBuf = Buffer.alloc(s, 65536, 2);
	var source;
	var fx = Synth(\dragonflyABVST, [\inbus, sourceBus, \outbus, 0]);
	var ctl = VSTPluginController(fx, \vst);
	var rec;
	var opened = Semaphore(0);
	s.sync;
	ctl.open(case[\key], editor: false, verbose: false, action: { |controller, ok|
		ok.if {
			controller.set(*case[\params]);
		} {
			("FAILED_OPEN " ++ case[\key]).error;
		};
		opened.signal;
	});
	opened.wait;
	0.2.wait;
	recBuf.write(path, "AIFF", "int24", 0, 0, true);
	s.sync;
	source = Synth.before(fx, \dragonflyABSource, [\out, sourceBus, \dur, duration]);
	rec = Synth.after(fx, \dragonflyABDiskOut, [\bufnum, recBuf.bufnum, \inbus, 0]);
	duration.wait;
	rec.free;
	fx.free;
	0.1.wait;
	recBuf.close;
	recBuf.free;
	s.sync;
	("AB_VST " ++ path).postln;
};

finish = {
	s.quit;
	0.exit;
};
)
