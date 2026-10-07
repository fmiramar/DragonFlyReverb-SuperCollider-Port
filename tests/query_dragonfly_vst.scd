(
var vstDir = "DRAGONFLY_VST3_DIR".getenv ? "/Library/Audio/Plug-Ins/VST3";
var paths = [vstDir];
var root = "DRAGONFLY_DF_ROOT".getenv;

root.isNil.if {
	root = PathName(thisProcess.nowExecutingPath).pathOnly +/+ "..";
};

s.options.numInputBusChannels = 0;
s.options.numOutputBusChannels = 2;
s.options.memSize = 262144;
s.options.ugenPluginsPath = [
	root +/+ "build-3.14.1/stage/DragonflyReverbDF",
	root +/+ "tests/vstplugin-scsynth-only",
	"/Applications/SuperCollider.app/Contents/Resources/plugins"
].join($:);

s.waitForBoot {
	VSTPlugin.search(s, paths, options: (save: false, parallel: false, timeout: 20), verbose: false, action: {
		VSTPlugin.pluginList(s, true).do { |desc|
			if(desc.path.asString.contains("Dragonfly")) {
				("PLUGIN_KEY " ++ desc.key).postln;
				("PLUGIN_PATH " ++ desc.path).postln;
				desc.parameters.do { |param, index|
					("PARAM " ++ index ++ " " ++ param[\name]).postln;
				};
			};
		};
		s.quit;
		0.exit;
	});
};
)
