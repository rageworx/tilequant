/**************************************/
#include <math.h>
#include <stdlib.h>
/**************************************/
#include "Colourspace.h"
#include "Dither.h"
#include "Qualetize.h"
/**************************************/

//! Palette entry matching
static int FindPaletteEntry(const struct RGBAf_t *Px, const struct RGBAf_t *Pal, int MaxPalSize, int PalUnused) 
{
	int   i;
	int   MinIdx = 0;
	float MinDst = INFINITY;
	struct RGBAf_t PxYUV = RGBAf_AsYUV(Px), PalYUV;
    
	for(i=PalUnused-1;i<MaxPalSize;i++) 
    {
		PalYUV = RGBAf_AsYUV(&Pal[i]);
		float Dst = RGBAf_ColDistance(&PxYUV, &PalYUV);
		if(Dst < MinDst) MinIdx = i, MinDst = Dst;
	}
    
	return MinIdx;
}

/**************************************/

//! Handle conversion of image with given palette, return RMS error
struct RGBAf_t DitherImage(
	const struct BmpCtx_t*  Image,
	const struct RGBA8_t*   BitRange,
	struct RGBAf_t*         RawPxOutput,

	int                     TileW,
	int                     TileH,
	int                     MaxTilePals,
	int                     MaxPalSize,
	int                     PalUnused,
	const int32_t*          TilePalIndices,
	const struct RGBAf_t*   TilePalettes,
	uint8_t*                TilePxOutput,

	int                     DitherType,
	float                   DitherLevel,
	struct RGBAf_t*         DiffusionBuffer
) {
	int i;

	//! Get parameters, pointers, etc.
	int x, y;
	int ImgW = Image->Width;
	int ImgH = Image->Height;
#if MEASURE_PSNR
	      struct RGBAf_t  RMSE     = (struct RGBAf_t){0,0,0,0};
#endif
	const        uint8_t *PxSrcIdx = Image->ColPal ? Image->PxIdx  : NULL;
	const struct RGBA8_t *PxSrcRGB = Image->ColPal ? Image->ColPal : Image->PxRGB;

	//! Initialize dither patterns
	//! For Floyd-Steinberg dithering, we only keep track of two scanlines
	//! of diffusion error (the current line and the next), and just swap
	//! back-and-forth between them to avoid a memcpy(). We also append an
	//! extra 2 pixels at the end of each line to avoid extra comparisons.
	union {
		struct RGBAf_t *DiffuseError;  //! DITHER_FLOYDSTEINBERG only
		struct RGBAf_t *PaletteSpread; //! DITHER_ORDERED only
		void *DataPtr;
	} Dither;
	Dither.DataPtr = DiffusionBuffer;
	if(DitherType != DITHER_NONE) 
    {
		if(DitherType == DITHER_FLOYDSTEINBERG) 
        {
			//! Error diffusion dithering
			for(i=0;i<(ImgW+2)*2;i++) Dither.DiffuseError[i] = (struct RGBAf_t){0,0,0,0};
		} 
        else 
        if(TilePxOutput) 
        {
			//! Ordered dithering (with tile palettes)
			for(i=0;i<MaxTilePals;i++) 
            {
				//! Find the mean values of this palette
				int n;
				struct RGBAf_t Mean = (struct RGBAf_t){0,0,0,0};
				for(n=PalUnused;n<MaxPalSize;n++) Mean = RGBAf_Add(&Mean, &TilePalettes[i*MaxPalSize+n]);
				Mean = RGBAf_Divi(&Mean, MaxPalSize-PalUnused);

				//! Compute slopes and store to the palette spread
				//! NOTE: For some reason, it works better to use the square root as a weight.
				//! This probably gives a value somewhere between the arithmetic mean and
				//! the smooth-max, which should result in better quality.
				//! NOTE: Pre-multiply by DitherLevel to remove a multiply from the main loop.
				struct RGBAf_t Spread = {0,0,0,0}, SpreadW = {0,0,0,0};
				for(n=PalUnused;n<MaxPalSize;n++) 
                {
					struct RGBAf_t d = RGBAf_Sub(&TilePalettes[i*MaxPalSize+n], &Mean);
						       d = RGBAf_Abs(&d);
					struct RGBAf_t w = RGBAf_Sqrt(&d);
						       d = RGBAf_Mul(&d, &w);
					Spread  = RGBAf_Add(&Spread,  &d);
					SpreadW = RGBAf_Add(&SpreadW, &w);
				}
				Spread = RGBAf_DivSafe(&Spread, &SpreadW, NULL);
#ifdef DITHER_NO_ALPHA
				Spread.a = 0.0f;
#endif
				Dither.PaletteSpread[i] = RGBAf_Muli(&Spread, DitherLevel);
			}
		} 
        else 
        {
			//! "Real" ordered dithering (without tile palettes)
			static const struct RGBA8_t MinValue = {1,1,1,1};
			struct RGBAf_t Spread = RGBAf_FromRGBA(&MinValue, BitRange);
			Dither.PaletteSpread[0] = RGBAf_Muli(&Spread, DitherLevel);
		}
	}

	//! Begin processing of pixels
	int TileHeightCounter = TileH;
	struct RGBAf_t *DiffuseThisLine = Dither.DiffuseError + 1;    //! <- 1px padding on left
	struct RGBAf_t *DiffuseNextLine = DiffuseThisLine + (ImgW+1); //! <- 1px padding on right
	struct RGBAf_t RMSE = (struct RGBAf_t){0,0,0,0};
	for(y=0;y<ImgH;y++) 
    {
		int TilePalIdx = 0;
		int TileWidthCounter = 0;
		for(x=0;x<ImgW;x++) 
        {
			//! Advance tile palette index
			if(TilePxOutput && --TileWidthCounter <= 0) 
            {
				TilePalIdx = *TilePalIndices++;
				TileWidthCounter = TileW;
			}

			//! Get pixel and apply dithering
			struct RGBAf_t Px, Px_Original; 
            {
				//! Read original pixel data
				struct RGBA8_t p;
				if(PxSrcIdx) p = PxSrcRGB[*PxSrcIdx++];
				else         p = *PxSrcRGB++;
				Px = Px_Original = RGBAf_FromRGBA8(&p);
			}
            
			if(DitherType != DITHER_NONE) 
            {
				if(DitherType == DITHER_FLOYDSTEINBERG) 
                {
					//! Adjust for diffusion error
					struct RGBAf_t t = DiffuseThisLine[x];
#ifdef DITHER_NO_ALPHA
					t.a = 0.0f;
#endif
					t  = RGBAf_Muli(&t, DitherLevel);
					Px = RGBAf_Add (&Px, &t);
				} 
                else 
                {
					//! Adjust for dither matrix
					int Threshold = 0, xKey = x, yKey = x^y;
					int Bit = DitherType-1; do {
						Threshold = Threshold*2 + (yKey & 1), yKey >>= 1; //! <- Hopefully turned into "SHR, ADC"
						Threshold = Threshold*2 + (xKey & 1), xKey >>= 1;
					} while(--Bit >= 0);
					float fThres = Threshold * (1.0f / (1 << (2*DitherType))) - 0.5f;
					struct RGBAf_t DitherVal = RGBAf_Muli(&Dither.PaletteSpread[TilePalIdx], fThres);
					Px = RGBAf_Add(&Px, &DitherVal);
				}
			}

			//! Find matching palette entry, store to output, and get error
			if(TilePxOutput) 
            {
				int PalIdx  = FindPaletteEntry(&Px, TilePalettes + TilePalIdx*MaxPalSize, MaxPalSize, PalUnused);
				    PalIdx += TilePalIdx*MaxPalSize;
				*TilePxOutput++ = PalIdx;
				Px = TilePalettes[PalIdx];
			} 
            else 
            {
				//! Reduce range when not using tile output
				struct RGBA8_t t = RGBA_FromRGBAf(&Px, BitRange);
				Px = RGBAf_FromRGBA(&t, BitRange);
			}
            
			if(RawPxOutput) 
            {
				*RawPxOutput++ = Px;
			}
            
			struct RGBAf_t Error = RGBAf_Sub(&Px_Original, &Px);

			//! Add to error diffusion
			if(DitherType == DITHER_FLOYDSTEINBERG) 
            {
				struct RGBAf_t t;

				//! {x+1,y} @ 7/16
				t = RGBAf_Muli(&Error, 7.0f/16);
				DiffuseThisLine[x+1] = RGBAf_Add(&DiffuseThisLine[x+1], &t);

				//! {x-1,y+1} @ 3/16
				t = RGBAf_Muli(&Error, 3.0f/16);
				DiffuseNextLine[x-1] = RGBAf_Add(&DiffuseNextLine[x-1], &t);

				//! {x+0,y+1} @ 5/16
				t = RGBAf_Muli(&Error, 5.0f/16);
				DiffuseNextLine[x+0] = RGBAf_Add(&DiffuseNextLine[x+0], &t);

				//! {x+1,y+1} @ 1/16
				t = RGBAf_Muli(&Error, 1.0f/16);
				DiffuseNextLine[x+1] = RGBAf_Add(&DiffuseNextLine[x+1], &t);
			}

			//! Accumulate error for RMS calculation
			Error = RGBAf_Mul(&Error, &Error);
			RMSE  = RGBAf_Add(&RMSE, &Error);
		}

		//! Advance tile palette index pointer
		//! At this point, we're already pointing to the next row of tiles,
		//! so we only need to either update the counter, or rewind the pointer.
		if(TilePxOutput) 
        {
			if(--TileHeightCounter <= 0) 
            {
				TileHeightCounter = TileH;
			} 
            else 
                TilePalIndices -= (ImgW/TileW);
		}

		//! Swap diffusion dithering pointers and clear buffer for next line
		if(DitherType == DITHER_FLOYDSTEINBERG) 
        {
			struct RGBAf_t *t = DiffuseThisLine;
			DiffuseThisLine = DiffuseNextLine;
			DiffuseNextLine = t;

			for(x=0;x<ImgW;x++) 
                DiffuseNextLine[x] = (struct RGBAf_t){0,0,0,0};
		}
	}

	//! Return error
	RMSE = RGBAf_Divi(&RMSE, ImgW*ImgH);
	RMSE = RGBAf_Sqrt(&RMSE);
	return RMSE;
}

/**************************************/
//! EOF
/**************************************/
