(
var root = "DRAGONFLY_DF_ROOT".getenv;
var vstDir = "DRAGONFLY_VST3_DIR".getenv ? "/Library/Audio/Plug-Ins/VST3";
var pluginPath;

var norm = { |value, lo, hi| ((value - lo) / (hi - lo)).clip(0, 1) };
var cases = [
	(
		name: "hall",
		key: "Dragonfly Hall Reverb.vst3",
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
		name: "room",
		key: "Dragonfly Room Reverb.vst3",
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

root.isNil.if {
	root = PathName(thisProcess.nowExecutingPath).pathOnly +/+ "..";
};

pluginPath = [
	root +/+ "build-3.14.1/stage/DragonflyReverbDF",
	root +/+ "tests/vstplugin-scsynth-only",
	"/Applications/SuperCollider.app/Contents/Resources/plugins"
].join($:);

s.options.numInputBusChannels = 0;
s.options.numOutputBusChannels = 2;
s.options.sampleRate = 48000;
s.options.blockSize = 64;
s.options.memSize = 262144;
s.options.ugenPluginsPath = pluginPath;

SynthDef(\dragonflyVSTProbe, {
	ReplaceOut.ar(0, VSTPlugin.ar(Silent.ar(2), 2, id: \vst));
}).add;

s.waitForBoot {
	VSTPlugin.search(s, vstDir, options: (save: false, parallel: false, timeout: 20), verbose: false, action: {
		Routine({
			cases.do { |case|
				var synth = Synth(\dragonflyVSTProbe);
				var ctl = VSTPluginController(synth, \vst);
				var opened = Semaphore(0);
				var got = Semaphore(0);
				ctl.open(case[\key], editor: false, verbose: false, action: { |controller, ok|
					("OPEN " ++ case[\name] ++ " " ++ ok).postln;
					controller.set(*case[\params]);
					opened.signal;
				});
				opened.wait;
				0.2.wait;
				ctl.getn(0, -1, { |values|
					("VALUES " ++ case[\name] ++ " " ++ values.collect(_.round(0.000001)).join(",")).postln;
					got.signal;
				});
				got.wait;
				synth.free;
				0.1.wait;
			};
			s.quit;
			0.exit;
		}).play(AppClock);
	});
};
)
