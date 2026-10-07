PlateDF : MultiOutUGen {
	*ar { arg inL = 0.0, inR = 0.0, dry = 80.0, wet = 20.0, algorithm = 1.0, width = 100.0, predelay = 0.0, decay = 0.4, lowCut = 200.0, highCut = 16000.0, damp = 13000.0;
		^this.multiNew('audio', 2, inL, inR, dry, wet, algorithm, width, predelay, decay, lowCut, highCut, damp)
	}

	init { arg numChannels ... theInputs;
		inputs = theInputs;
		^this.initOutputs(numChannels, rate);
	}

	argNamesInputsOffset { ^2 }
}

