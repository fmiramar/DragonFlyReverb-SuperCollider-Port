RoomDF : MultiOutUGen {
	*ar { arg inL = 0.0, inR = 0.0, dry = 80.0, early = 10.0, earlySend = 20.0, late = 20.0, size = 12.0, width = 100.0, predelay = 8.0, decay = 0.4, diffuse = 70.0, spin = 0.8, wander = 40.0, inHighCut = 16000.0, earlyDamp = 10000.0, lateDamp = 9400.0, boost = 50.0, boostLPF = 600.0, inLowCut = 4.0;
		^this.multiNew('audio', 2, inL, inR, dry, early, earlySend, late, size, width, predelay, decay, diffuse, spin, wander, inHighCut, earlyDamp, lateDamp, boost, boostLPF, inLowCut)
	}

	init { arg numChannels ... theInputs;
		inputs = theInputs;
		^this.initOutputs(numChannels, rate);
	}

	argNamesInputsOffset { ^2 }
}

