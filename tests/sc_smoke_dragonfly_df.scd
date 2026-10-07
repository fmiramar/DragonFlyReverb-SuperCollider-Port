(
var root = "DRAGONFLY_DF_ROOT".getenv;
var oscPath;
var score;

root.isNil.if {
	root = PathName(thisProcess.nowExecutingPath).pathOnly +/+ "..";
};
oscPath = root +/+ "build-3.14.1/smoke-dragonfly-df.osc";

score = Score([
	[0.0, [\d_recv, SynthDef(\dragonflyDFEarlySmoke, {
		var src = Decay2.ar(Impulse.ar(2), 0.001, 0.04) * PinkNoise.ar(0.25);
		var sig = EarlyReflectionsDF.ar(src, src, dry: 20, wet: 60, program: 2, size: 20, width: 110);
		Out.ar(0, sig);
	}).asBytes]],
	[0.0, [\d_recv, SynthDef(\dragonflyDFHallSmoke, {
		var src = Decay2.ar(Impulse.ar(1), 0.001, 0.06) * WhiteNoise.ar(0.18);
		var sig = HallDF.ar(src, src, dry: 20, early: 20, late: 40, size: 24, width: 100, predelay: 8, decay: 1.4);
		Out.ar(0, sig);
	}).asBytes]],
	[0.0, [\d_recv, SynthDef(\dragonflyDFPlateSmoke, {
		var src = Decay2.ar(Impulse.ar(1.5), 0.001, 0.05) * PinkNoise.ar(0.2);
		var sig = PlateDF.ar(src, src, dry: 20, wet: 45, algorithm: 1, width: 100, predelay: 5, decay: 0.8);
		Out.ar(0, sig);
	}).asBytes]],
	[0.0, [\d_recv, SynthDef(\dragonflyDFRoomSmoke, {
		var src = Decay2.ar(Impulse.ar(1.25), 0.001, 0.05) * WhiteNoise.ar(0.16);
		var sig = RoomDF.ar(src, src, dry: 20, early: 15, earlySend: 20, late: 35, size: 12, width: 100, predelay: 6, decay: 0.5);
		Out.ar(0, sig);
	}).asBytes]],
	[0.0, [\s_new, \dragonflyDFEarlySmoke, 1000, 0, 1]],
	[0.75, [\n_free, 1000]],
	[0.75, [\s_new, \dragonflyDFHallSmoke, 1001, 0, 1]],
	[1.75, [\n_free, 1001]],
	[1.75, [\s_new, \dragonflyDFPlateSmoke, 1002, 0, 1]],
	[2.75, [\n_free, 1002]],
	[2.75, [\s_new, \dragonflyDFRoomSmoke, 1003, 0, 1]],
	[3.75, [\n_free, 1003]],
	[4.0, [0]]
]);

score.writeOSCFile(oscPath, 0, 4.0);
("WROTE " ++ oscPath).postln;
0.exit;
)
