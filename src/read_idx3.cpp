#include <iostream>
#include <fstream>
#include <stdio.h>
#include <vector>

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
};

void drawImage(uint8_t* data, int xlen, int ylen)
{
	const char* vals = " .,oO0*&$";
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

int main(int argc, char** argv)
{
	if( argc < 2 )
	{
		std::cout << "please supply a IDX3 filename as argument" << std::endl;
		return -1;
	}

	FILE* fp = fopen(argv[1], "rb");
	if( !fp)
	{
		std::cout << "failed to open " << argv[1] << std::endl;
		return -1;
	}

	uint8_t magic[4];
	fread(magic, 4, 1,fp);
	
	std::vector<IdxData> data;
	if (magic[0] == 0 && magic[1] == 0 )
	{
		size_t size = 1;
		if( eFormat fmt  = getFormat(magic[2], size) )
		{
			int dim = magic[3];
			print_Format(fmt, dim, size);	
			
			IdxData d;
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
				d.dims.push_back(dimSize);
			}
			for(int i : d.dims )
			{
				s*= i;	
			}
			std::cout << "reading " << (s/(1024*1024)) << " mb" << std::endl;
			d.bytes.resize(s);
			fread(d.bytes.data(), s, 1, fp);

			for( int i = 0; i < 60000; ++i )
			{
				size_t offset = i*d.dims[1]*d.dims[2];
				drawImage(d.bytes.data()+offset, d.dims[1], d.dims[2]);			
			}
		}
	}

	fclose(fp);
	return 0;
}
