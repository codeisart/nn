#include <iostream>
#include <fstream>
#include <stdio.h>
#include <vector>
#include <stdlib.h>
#include <cmath>


enum eFormat
{
	FMT_invalid = 0,
	FMT_ubyte=0x08, // 1 byte
	FMT_sbyte=0x09, // 1 byte
	FMT_short=0x0B,	// 2 byte
	FMT_int=0x0C,	// 4 byte
	FMT_float=0x0D,	// 4 byte
	FMT_double=0x0E // 8 bytes
};

int32_t SwapBytes(int32_t value)
{
	int32_t retval;
	retval=value & 0xFF;
	retval=(retval<<8) | ((value >>8)& 0xFF);
	retval=(retval<<8) | ((value >>16)& 0xFF);
	retval=(retval<<8) | ((value >>24)& 0xFF); 
	return retval;
}


eFormat getFormat(int type, size_t& out_dataSizeInBytes)
{
	switch( type )
	{
		case FMT_ubyte: // 1 byte
		case FMT_sbyte: // 1 byte
			out_dataSizeInBytes = 1;
			break;
		case FMT_short:	// 2 byte
			out_dataSizeInBytes = 2;
			break;
		case FMT_int:	// 4 byte
		case FMT_float:	// 4 byte
			out_dataSizeInBytes = 4;
			break;
		case FMT_double: // 8 bytes
			out_dataSizeInBytes = 8;
			break;
		default:
		{
			std::cout << "invalid type" << std::endl;
			return FMT_invalid;
		}
	}

	return (eFormat)type;
}

const char* format_toString(eFormat fmt)
{
	switch(fmt)
	{
		case FMT_ubyte: return "ubyte";
		case FMT_sbyte: return "sbyte";
		case FMT_short:	return "short";
		case FMT_int:	return "int";
		case FMT_float:	return "float";
		case FMT_double: return "double";
		default: 
			return "unknown";
	}
}
	
void print_Format(eFormat fmt, int dim, int size)
{
	std::cout << "fmt=" << format_toString(fmt) << "\t"  
		  << "size=" << size << "\t" 
		  << "dim=" << dim << "\t"
		  << std::endl;
}

struct IdxData
{
	std::vector<uint8_t> bytes;
	std::vector<int> dims;

	bool load(const char* pFilename)
	{
		FILE* fp = fopen(pFilename, "rb");
		if( !fp)
		{
			std::cout << "failed to open " << pFilename << std::endl;
			return false;
		}

		uint8_t magic[4];
		fread(magic, 4, 1,fp);

		if (magic[0] == 0 && magic[1] == 0 )
		{
			size_t size = 1;
			if( eFormat fmt  = getFormat(magic[2], size) )
			{
				int dim = magic[3];
				print_Format(fmt, dim, size);	

				size_t s = 1;
				for(int i= 0;i<dim; ++i)
				{
					// read size.
					int32_t dimSize = 0;
					fread(&dimSize,1,4,fp);

					// byteswap
					dimSize = SwapBytes(dimSize);

					// debug.
					std::cout << "\t" << i << "=" << dimSize << std::endl;

					// record size.
					dims.push_back(dimSize);
				}
				for(int i : dims )
				{
					s*= i;	
				}
				std::cout << "reading " << (s/(1024*1024)) << " mb" << std::endl;
				bytes.resize(s);
				fread(bytes.data(), s, 1, fp);

				return true;
			}
		}
		return false;

	}

};

void drawImage(uint8_t* data, int xlen, int ylen)
{
	const char* vals = " .:-=+*#%@";
	
	for(int y=0; y<ylen; ++y)
	{
		for(int x=0; x<xlen; ++x)
		{
			int val = data[(y*xlen)+x]/32;
			std::cout << vals[val];
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;
}

IdxData gLabels;
IdxData gImgs;

struct Layer
{
	std::vector<float> v; 			// vals;
	std::vector<std::vector<float>> w; 	// weights.
	std::vector<float> b; 			// bias.

	static float randNorm() // value between -1...1
	{
		static int kMax = 65535;
		int r = rand() % kMax;
		float fr = (float)r / kMax;
		return fr;
	}

	static float calcPass(std::vector<float>& inputs, std::vector<float>& weights, float bias)
	{
		// apply weights.
		float val = 0;
		for(int i = 0; i < inputs.size(); ++i)
		{
			val += weights[i] * inputs[i];
		}

		// add bias.
		val += bias;

		// activation funciton.
		val = sigmoid(val);

		return val;
	}

	void init(int nVals, int nInputs)
	{
		v.resize(nVals);
		w.resize(nVals);
		b.resize(nVals);

		// foreach neuron. 
		for(int n = 0; n < v.size(); n++)
		{
			// for each input.
			b[n] = randNorm();
			std::vector<float>& weights = w[n];
			weights.resize(nInputs);
			for(int i = 0; i < nInputs; ++i)
			{
				weights[i] = randNorm();
			}
		}

	}

	static float sigmoid(float x)
	{
		return x / (1.f + fabs(x));
	}

	void doForwardPass(std::vector<float>& in)
	{
		// Each neuron has a weight for each input
		for(int n = 0; n < v.size(); n++)
		{
			v[n] = calcPass(in,w[n],b[n]);
		}
	}

	void calcLoss(std::vector<float>& expected, std::vector<float>& deltaSqrd)
	{
		deltaSqrd.resize(expected.size());
		for(int i = 0; i < expected.size(); ++i)
		{
			float d = v[i]-expected[i];
			deltaSqrd[i] = d*d;	
		}
	}	

	Layer(int nNodes, int nInputs ) { init(nNodes,nInputs); }	
};

struct NN
{
	// input layer.
	std::vector<float> input;

	std::vector<Layer> hidden;

	// output layer
	Layer output;

	void doForwardPass()
	{
		hidden[0].doForwardPass(input);
		hidden[1].doForwardPass(hidden[0].v);	
		output.doForwardPass(hidden[1].v);	
	}
	void calcLoss(std::vector<float>& expected, std::vector<float>& loss)
	{
		output.calcLoss(expected,loss);
	}

	NN(int nInputs, int nHidden, int nOutputs)
		: output(16, nOutputs)
	{
		input.resize(nInputs);
		hidden.resize(nHidden, Layer(16, nInputs)) ;
	}
};

NN gNN(28*28, 2, 10);

int main(int argc, char** argv)
{
	if( argc < 3 )
	{
		std::cout << "usage: training_images.idx training_lables.idx " << std::endl;
		return -1;
	}

	gImgs.load(argv[1]);
	gLabels.load(argv[2]);

	for( int i = 0; i < gImgs.dims[0]; ++i )
	{
		size_t offset = i*gImgs.dims[1]*gImgs.dims[2];
		drawImage(gImgs.bytes.data()+offset, gImgs.dims[1], gImgs.dims[2]);			
		int labl = gLabels.bytes[i];
		std::cout << labl << std::endl;

		// light up correct output.
		std::vector<float> expected;
		expected.resize(10,0.f);
		expected[labl] = 1.f;

		// send input to NN.
		uint8_t* pSrc = gImgs.bytes.data()+offset;
		float* pDst = gNN.input.data();
		for(int y = 0; y < gImgs.dims[1]; ++y)
		{
			for(int x = 0; x < gImgs.dims[2]; x++)
			{
				*pDst = ((float)*pSrc/255.f); // put intput into 0..1 range.
				pDst++;
				pSrc++;
			}
		}

		gNN.doForwardPass();
		std::vector<float> loss;
		loss.resize(10);
		gNN.calcLoss(expected,loss);

		std::cout << "error={";
		for( float i : loss)
			std::cout << i << ",";
		std::cout << "}" << std::endl;
		
		std::cout << "output={";
		for( float i : gNN.output.v)
			std::cout << i << ",";
		std::cout << "}" << std::endl;

		// Debug.
		//break;
	}
	/*
	for( int i = 0; i < gImgs.dims[0]; ++i )
	{
		size_t offset = i*gImgs.dims[1]*gImgs.dims[2];
		drawImage(gImgs.bytes.data()+offset, gImgs.dims[1], gImgs.dims[2]);			
		int labl = gLabels.bytes[i];
		std::cout << labl << std::endl;
	}
	*/

	return 0;
}
