/**************************************/
#include <stdlib.h>
/**************************************/
#include "Bitmap.h"
#include "Colourspace.h"
#include "Dither.h"
#include "Qualetize.h"
#include "Tiles.h"
/**************************************/

//! Handle conversion of image with given palette, return RMS error
//! NOTE: Lots of pointer aliasing to avoid even more memory consumption
struct RGBAf_t Qualetize(
	struct BmpCtx_t*        Image,
	struct TilesData_t*     TilesData,
	uint8_t*                PxData,
	struct RGBAf_t*         Palette,
	int                     MaxTilePals,
	int                     MaxPalSize,
	int                     PalUnused,
	int                     nTileClusterPasses,
	int                     nColourClusterPasses,
	const struct RGBA8_t*   BitRange,
	int                     DitherType,
	float                   DitherLevel,
	int                     ReplaceImage
) {
	int i;

	//! Do palette allocation and colour clustering
	TilesData_QuantizePalettes(
		TilesData,
		Palette,
		MaxTilePals,
		MaxPalSize,
		PalUnused,
		nTileClusterPasses,
		nColourClusterPasses
	);

	//! Convert palette to RGBA and reduce range
	for(i=0;i<MaxTilePals*MaxPalSize;i++) 
    {
		struct RGBAf_t p  = RGBAf_FromYUV(&Palette[i]);
		struct RGBA8_t p2 = RGBA_FromRGBAf(&p, BitRange);
		Palette[i] = RGBAf_FromRGBA(&p2, BitRange);
	}

	//! Do final dithering+palette processing
	struct RGBAf_t RMSE = DitherImage(
		Image,
		BitRange,
		NULL,
		TilesData->TileW,
		TilesData->TileH,
		MaxTilePals,
		MaxPalSize,
		PalUnused,
		TilesData->TilePalIdx,
		Palette,
		PxData,
		DitherType,
		DitherLevel,
		TilesData->PxTemp
	);

	//! Store the final palette
	//! NOTE: This aliases over the original palette, but is
	//! safe because RGBA8_t is smaller than RGBAf_t
	struct RGBA8_t *PalBGR = (struct RGBA8_t*)Palette;
	for(i=0;i<BMP_PALETTE_COLOURS;i++) 
    {
		PalBGR[i] = RGBA8_FromRGBAf(&Palette[i]);
	}

	//! Store new image data
	if(ReplaceImage) 
    {
		if(Image->ColPal) 
        {
			free(Image->ColPal);
			free(Image->PxIdx);
		} 
        else 
            free(Image->PxRGB);
        
		Image->ColPal = PalBGR;
		Image->PxIdx  = PxData;
	}

	//! Return error
	return RMSE;
}

/**************************************/
//! EOF
/**************************************/
