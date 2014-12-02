// To avoid warnings in MSVC
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <Windows.h>
// buffer (input), whole file (both files), buffer (impulse)
char *buffer, *wF, *bufferM;
// input, impulse, and output signals respectively
short *signals, *msig, *osig;
// buffer and signal length for input & impulse
int blen, slen, blenM, slenM;
// linked to from the assembly file
void normalize(double*, short*, int);
void swap(double*, double*);
int max2(int, int);
// PI * 2
const double PI2 = 6.28318530717959;
// Variables needed from input & impulse WAV files (M suffix is for impulse file)
int samplerate, datasize, datasizeM, signalsizeM;
short channels, bitspersample, bitspersampleM;

/**
 * Finds the string "data" in a byte array
 * @param buffer The buffer containing the bytes to search
 * @param len The length of the array
 * @return The index immediately after the word "data" ends
 */
int findDATA(char *buffer, int len)
{
	int i;
	for (i = 0; i < len - 5; i++)
	{
		if (buffer[i] == 'd' && buffer[i + 1] == 'a' && buffer[i + 2] == 't' && buffer[i + 3] == 'a')
		{
			return i + 4;
		}
	}
	return -1;
}

/**
 * Checks for validity of WAV file
 * @param fp The file to check
 * @return 1 if WAV; 0 otherwise
 */
int isWAV(FILE *fp)
{
	int magic;
	rewind(fp);
	fread(&magic, 4, 1, fp);
	if (magic == (('R') | ('I' << 8) | ('F' << 16) | ('F' << 24)))
	{
		return 1;
	}
	return 0;
}

/**
 * Reads the input files and populates the global state variables with their info.
 * @param filename The filename to open and read
 * @param type If 1, this is the input file. If 2, this is the impulse file.
 */
void readFile(char *filename, int type)
{
	if (type == 1)
	{
		FILE *fp;
		int flen, dloc;
		fp = fopen(filename, "rb");
		if (fp == NULL)
		{
			printf("%s could not be found.\n", filename);
			return;
		}
		if (!isWAV(fp))
		{
			printf("%s is not a valid RIFF file.\n", filename);
			return;
		}
		fseek(fp, 0, SEEK_END);
		flen = ftell(fp);
		rewind(fp);
		
		wF = (char*)malloc(flen);
		fread(wF, flen, 1, fp);
		rewind(fp);
		fseek(fp, 22, SEEK_SET);
		fread((char*)&channels, 2, 1, fp);
		fseek(fp, 24, SEEK_SET);
		fread((char*)&samplerate, 4, 1, fp);
		fseek(fp, 34, SEEK_SET);
		fread((char*)&bitspersample, 2, 1, fp);
		dloc = findDATA(wF, flen);
		fseek(fp, dloc, SEEK_SET);
		fread((char*)&datasize, 4, 1, fp);
		free(wF);
		buffer = (char*)malloc(datasize);
		fseek(fp, dloc+4, SEEK_SET);
		fread(buffer, datasize, 1, fp);
		fclose(fp);
		blen = datasize;
		if (bitspersample == 8)
		{
			int i;
			signals = (short*)malloc(datasize*sizeof(short));
			if (signals == NULL)
			{
				printf("Malloc failed (1)\n");
				return;
			}
			slen = datasize;
			for (i = 0; i < datasize; i++) signals[i] = (short)((unsigned char)(buffer[i]));
		}
		else if (bitspersample == 16)
		{
			int i;
			short accum;
			signals = (short*)malloc((datasize / 2) * sizeof(short));

			if (signals == NULL)
			{
				printf("Malloc failed (2) (datasize/2)=%d\n", datasize/2);
				return;
			}
			slen = datasize / 2;
			for (i = 0; i < datasize; i += 2)
			{
				accum = (short)((unsigned char)buffer[i]);
				accum += (short)((unsigned char)buffer[i + 1]) * 256;
				signals[i / 2] = accum;
			}
		}
		else
		{
			printf("Unsupported bit rate\n");
			return;
		}
		//free(buffer);
	}
	else if (type == 2)
	{
		FILE *fp;
		int flen, dloc;
		fp = fopen(filename, "rb");
		if (fp == NULL)
		{
			printf("%s could not be found.\n", filename);
			return;
		}
		if (!isWAV(fp))
		{
			printf("%s is not a valid RIFF file.\n", filename);
			return;
		}
		fseek(fp, 0, SEEK_END);
		flen = ftell(fp);
		rewind(fp);
		wF = (char*)malloc(flen);
		dloc = findDATA(wF, flen);
		free(wF);
		fseek(fp, 34, SEEK_SET);
		fread((char*)&bitspersampleM, 2, 1, fp);

		fseek(fp, dloc, SEEK_SET);
		fread((char*)&datasizeM, 4, 1, fp);
		bufferM = (char*)malloc(datasizeM);
		fseek(fp, dloc+4, SEEK_SET);
		fread(bufferM, datasizeM, 1, fp);
		fclose(fp);
		blenM = datasizeM;

		if (bitspersampleM == 8)
		{
			int i;
			msig = (short*)malloc(datasizeM*sizeof(short));
			if (msig == NULL)
			{
				printf("Malloc failed (3)\n");
				return;
			}
			slenM = datasizeM;
			for (i = 0; i < datasizeM; i++) msig[i] = (short) ((unsigned char)bufferM[i]);
		}
		else if (bitspersampleM == 16)
		{

			int i;
			short accum;
			msig = (short*)malloc((datasizeM / 2) * sizeof(short));
			if (msig == NULL)
			{
				printf("Malloc failed (4)\n");
				return;
			}
			slenM = datasizeM / 2;
			for (i = 0; i < datasizeM; i += 2)
			{
				accum = (short)((unsigned char)bufferM[i]);
				accum += (short)((unsigned char)bufferM[i + 1]) * 256;
				msig[i / 2] = accum;
			}
		}
		else
		{
			printf("Unsupported bit rate\n");
			return;
		}
		//free(bufferM);
	}
}

// Source: Numerical Recipes in C, p. 507-508
// Four:1 FFT algorithm. 
// Didn't like overlap-add or input-side; both were very slow
// This algorithm is faster (by far).
// Differences from textbook version:
// 1. Uses my assembler swap function (in fastfunctions.asm) rather than the macro
//	   this is to optimize the running speed.
// 2. The INCR constant is 2
// 3. 
void four1(double *data, int nn, int isign)
{
	unsigned long n, mmax, m, j, istep, i;
	double wtemp, wr, wpr, wpi, wi, theta;
	double tempr, tempi;
	const short INCR = 2;
	n = nn << 1;
	j = 1;

	for (i = 1; i < n; i += INCR)
	{
		if (j > i) 
		{
			swap(&data[j], &data[i]);
			swap(&data[j + 1], &data[i + 1]);
		}
		m = nn;
		while (m >= 2 && j > m)
		{
			j -= m;
			m >>= 1;
		}
		j += m;
	}

	mmax = 2;

	while (n > mmax) 
	{
		istep = mmax << 1;
		theta = isign * (PI2 / mmax);
		wtemp = sin(0.5 * theta);
		wpr = -2.0 * wtemp * wtemp;
		wpi = sin(theta);
		wr = 1.0;
		wi = 0.0;
		for (m = 1; m < mmax; m += INCR) 
		{
			for (i = m; i <= n; i += istep) 
			{
				j = i + mmax;
				tempr = wr * data[j] - wi * data[j + 1];
				tempi = wr * data[j + 1] + wi * data[j];
				data[j] = data[i] - tempr;
				data[j + 1] = data[i + 1] - tempi;
				data[i] += tempr;
				data[i + 1] += tempi;
			}
			wr = (wtemp = wr) * wpr - wi * wpi + wr;
			wi = wi * wpr + wtemp * wpi + wi;
		}
		mmax = istep;
	}
}

/**
 * Little-endian write integer
 * Writes an 32-bit integer to a file in little-endian format (regardless of machine endianness).
 * @param in The integer to write
 * @param fp The pointer to an already open file.
 */
void lewINT(int in, FILE *fp)
{
	uint8_t mem;
	mem = in & 0xff;
	fwrite(&mem, 1, 1, fp);
	mem = (in >> 8) & 0xff;
	fwrite(&mem, 1, 1, fp);
	mem = (in >> 16) & 0xff;
	fwrite(&mem, 1, 1, fp);
	mem = (in >> 24) & 0xff;
	fwrite(&mem, 1, 1, fp);
}

/**
 * Little-endian write short
 * Writes a 16-bit  integer to a file in little-endian format (regardless of machine endianness).
 * @param in The short to write
 * @param fp The pointer to an already open file.
 */
void lewSHORT(short in, FILE *fp)
{
	uint8_t mem;
	mem = in & 0xff;
	fwrite(&mem, 1, 1, fp);
	mem = (in >> 8) & 0xff;
	fwrite(&mem, 1, 1, fp);
}

/**
 * Writes the RIFF header to a file.
 * @param channels The number of channels that will be in the file
 * @param nsamp The number of samples that will be in the file
 * @param bps The bits per second that will be in the file
 * @param srate The sample rate that will be in the file
 */
void writeWAVHdr(int channels, int nsamp, int bps, double srate, FILE *fp)
{
	int dcsz = channels * nsamp * (bps / 8);
	int formlen = 36 + dcsz;
	short framelen = channels * (bps / 8);
	int byps = (int)ceil(srate*framelen);
	int mem;
	
	mem = (('R') | ('I' << 8) | ('F' << 16) | ('F' << 24));
	fwrite(&mem, 4, 1, fp);
	lewINT(formlen, fp);
	mem = (('W') | ('A' << 8) | ('V' << 16) | ('E' << 24));
	fwrite(&mem, 4, 1, fp);
	mem = (('f') | ('m' << 8) | ('t' << 16) | (' ' << 24));
	fwrite(&mem, 4, 1, fp);
	lewINT(16, fp);
	lewSHORT(1, fp);
	lewSHORT((short)channels, fp);
	lewINT((int)srate, fp);
	lewINT(byps, fp);
	lewSHORT(framelen, fp);
	lewSHORT(bps, fp);
	mem = (('d') | ('a' << 8) | ('t' << 16) | ('a' << 24));
	fwrite(&mem, 4, 1, fp);
	lewINT(dcsz, fp);

}
/**
 * Writes out a complete WAV file.
 * @param sig An array containing all the audio signals in the file.
 * @param nsamp The number of samples in the file.
 * @param fname The name of the file to attempt to write.
 */
void writeWAV(double *sig, int nsamp, char *fname)
{
	FILE *fp = fopen(fname, "wb");
	int i;
	double maxL = -1.0, maxG = -1.0;
	if (fp == NULL)
	{
		printf("Could not open %s\n", fname);
		return;
	}
	
	writeWAVHdr(channels, nsamp, bitspersample, samplerate, fp);
	
	for (i = 0; i < nsamp; i++)
	{
		if (sig[i] > maxL) maxL = sig[i];
	}
	for (i = 0; i < slen; i++)
	{
		if (signals[i] > maxG) maxG = signals[i];
	}

	for (i = 0; i < nsamp; i++)
	{
		lewSHORT((short)(sig[i] / maxL * maxG), fp);
	}
	
	fclose(fp);
	

}

/**
 * Main execution method.
 * Reads arguments and passes them to the FFT convolution functions, and writes out the output file.
 * @param argc Number of arguments; in our case, should be at least 4.
 * @param argv The actual arguments; 0 => program name; 1 => input file (must exist); 2 => impulse file (must exist); 3 => output file (overwritten if exists)
 * @return 0 if no errors occured; -1 otherwise.
 */
int main(int argc, char *argv[])
{
	double *h, *x, *hC, *xC, *oC;
	int i, ipow, ipow2;
	DWORD start, end;
	if (argc < 4)
	{
		printf("Usage: convolve <input-file> <impulse-file> <output-filename>\n");
		return 0;
	}
	readFile(argv[1], 1);
	readFile(argv[2], 2);
	printf("Files are valid; performing computations...\n");
	start = GetTickCount();
	h = (double*)malloc(slenM*sizeof(double));
	x = (double*)malloc(slen*sizeof(double));
	if (h == NULL || x == NULL)
	{
		printf("Malloc failed %d\n", __LINE__);
		return -1;
	}
	normalize(h, msig, slenM);
	normalize(x, signals, slen);
	ipow = (int)log2(max2(slen, slenM)) + 1;
	ipow = (int)pow(2, ipow);
	ipow2 = 2* ipow;
	hC = (double*)malloc(ipow2*sizeof(double));
	xC = (double*)malloc(ipow2*sizeof(double));
	for (i = 0; i < ipow2; i++)
	{
		hC[i] = 0.0;
		xC[i] = 0.0;
	}
	for (i = 0; i < slenM; i++)
	{
		hC[2 * i] = h[i];
	}
	for (i = 0; i < slen; i++)
	{
		xC[2 * i] = x[i];
	}
	four1(hC-1, ipow, 1);
	four1(xC-1, ipow, 1);
	oC = (double*)malloc(ipow2*sizeof(double));
	for (i = 0; i < ipow; i++)
	{
		oC[i * 2]		= xC[i]		* hC[i]	- xC[i + 1] * hC[i + 1];
		oC[i * 2 + 1]	= xC[i + 1] * hC[i] + xC[i]		* hC[i + 1];
	}
	four1(oC - 1, ipow, -1);
	end = GetTickCount();
	printf("Finished in %d milliseconds.\n", end - start);
	writeWAV(oC, ipow, argv[3]);
	return 0;
}