HallDF : MultiOutUGen {
	*ar { arg inL = 0.0, inR = 0.0, dry = 80.0, early = 10.0, late = 20.0, size = 24.0, width = 100.0, predelay = 4.0, diffuse = 90.0, lowCut = 4.0, lowXover = 500.0, lowMult = 1.3, highCut = 7600.0, highXover = 5500.0, highMult = 0.5, spin = 3.3, wander = 15.0, decay = 1.3, earlySend = 20.0, modulation = 15.0;
		^this.multiNew('audio', 2, inL, inR, dry, early, late, size, width, predelay, diffuse, lowCut, lowXover, lowMult, highCut, highXover, highMult, spin, wander, decay, earlySend, modulation)
	}

	init { arg numChannels ... theInputs;
		inputs = theInputs;
		^this.initOutputs(numChannels, rate);
	}

	argNamesInputsOffset { ^2 }
}

