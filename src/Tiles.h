/**************************************/
#pragma once
/**************************************/
#include <stdint.h>
/**************************************/
#include "Bitmap.h"
#include "Colourspace.h"
/**************************************/

union TilePx_t 
{
	struct RGBAf_t*    PxRGBAf;
	struct RGBA8_t *    PxRGBA8;
	uint8_t PxIdx;
};

struct TilesData_t 
{
	int                 TileW;
    int                 TileH;
	int                 TilesX;
    int                 TilesY;
	union TilePx_t*     TilePxPtr;  //! Tile pixel pointers
	struct RGBAf_t*     TileValue;  //! Tile values (for quantization comparisons)
	struct RGBAf_t*     PxData;     //! Tile pixel data           (ImageW*ImageH elements)
	struct RGBAf_t*     PxTemp;     //! Temporary processing data (ImageW*ImageH elements)
	int32_t*            PxTempIdx;  //! Temporary processing data (palette entry indices)
	int32_t*            TilePalIdx; //! Tile palette indices
};

/**************************************/

//! Convert bitmap to tiles
//! NOTE: To destroy, call free() on the returned pointer
struct TilesData_t *TilesData_FromBitmap
(
	const struct BmpCtx_t*  Ctx,
	int                     TileW,
	int                     TileH,
	const struct RGBA8_t*   BitRange,
	int                     DitherType,
	float                   DitherLevel
);

//! Create quantized palette
//! NOTE: PalUnusedEntries is used for 'padding', such as on
//! the GBA/NDS where index 0 of every palette is transparent
//! NOTE: Palette is generated in YUVA mode
int TilesData_QuantizePalettes
(
	struct TilesData_t* TilesData,
	struct RGBAf_t*     Palette,
	int                 MaxTilePals,
	int                 MaxPalSize,
	int                 PalUnusedEntries,
	int                 nTileClusterPasses,
	int                 nColourClusterPasses
);

/**************************************/
//! EOF
/**************************************/
