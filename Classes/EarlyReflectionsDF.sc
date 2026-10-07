EarlyReflectionsDF : MultiOutUGen {
	*ar { arg inL = 0.0, inR = 0.0, dry = 80.0, wet = 20.0, program = 2.0, size = 20.0, width = 100.0, lowCut = 50.0, highCut = 10000.0;
		^this.multiNew('audio', 2, inL, inR, dry, wet, program, size, width, lowCut, highCut)
	}

	init { arg numChannels ... theInputs;
		inputs = theInputs;
		^this.initOutputs(numChannels, rate);
	}

	argNamesInputsOffset { ^2 }
}

