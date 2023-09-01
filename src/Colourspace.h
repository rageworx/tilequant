/**************************************/
#pragma once
/**************************************/
#include <math.h>
#include <stdint.h>
/**************************************/
#define COLOURSPACE_CLIP(x, Min, Max) ((x) < (Min) ? (Min) : (x) > (Max) ? (Max) : (x))
/**************************************/

struct RGBA8_t { uint8_t r, g, b, a; };
struct RGBAf_t { float   r, g, b, a; };

/**************************************/

static inline struct RGBA8_t RGBA_FromRGBAf(const struct RGBAf_t *x, const struct RGBA8_t *Range) 
{
	struct RGBA8_t Out;
	Out.r = (uint8_t)COLOURSPACE_CLIP((x->r*Range->r + 0.5f), 0, Range->r);
	Out.g = (uint8_t)COLOURSPACE_CLIP((x->g*Range->g + 0.5f), 0, Range->g);
	Out.b = (uint8_t)COLOURSPACE_CLIP((x->b*Range->b + 0.5f), 0, Range->b);
	Out.a = (uint8_t)COLOURSPACE_CLIP((x->a*Range->a + 0.5f), 0, Range->a);
	return Out;
}

static inline struct RGBA8_t RGBA8_FromRGBAf(const struct RGBAf_t *x) 
{
	return RGBA_FromRGBAf(x, &(struct RGBA8_t){255,255,255,255});
}

/**************************************/

static inline struct RGBAf_t RGBAf_FromRGBA(const struct RGBA8_t *x, const struct RGBA8_t *Range) 
{
	struct RGBAf_t Out = 
    {
		x->r / (float)Range->r,
		x->g / (float)Range->g,
		x->b / (float)Range->b,
		x->a / (float)Range->a
	};
	return Out;
}

static inline struct RGBAf_t RGBAf_FromRGBA8(const struct RGBA8_t *x) 
{
	return RGBAf_FromRGBA(x, &(struct RGBA8_t){255,255,255,255});
}

/**************************************/

//! x->b = Y
//! x->g = Cb
//! x->r = Cr
//! Using ITU-R BT.709 constants
static inline struct RGBAf_t RGBAf_AsYUV(const struct RGBAf_t *x) 
{
	return (struct RGBAf_t) \
    {
		 0.2126f*x->r + 0.71520f*x->g + 0.0722f*x->b,
		-0.1146f*x->r - 0.38540f*x->g + 0.5000f*x->b,
		 0.5f   *x->r - 0.45420f*x->g - 0.0458f*x->b,
		x->a
	};
}
static inline struct RGBAf_t RGBAf_FromYUV(const struct RGBAf_t *x) {
	return (struct RGBAf_t) \
    {
		x->b + 1.855609686f*x->g + 0.000105740f*x->b,
		x->b - 0.187280216f*x->g - 0.468124625f*x->r,
		x->b - 0.000151501f*x->g + 1.574765276f*x->r,
		x->a
	};
}

/**************************************/

static inline struct RGBAf_t RGBAf_Add(const struct RGBAf_t *a, const struct RGBAf_t *b) 
{
	struct RGBAf_t Out;
	Out.r = a->r + b->r;
	Out.g = a->g + b->g;
	Out.b = a->b + b->b;
	Out.a = a->a + b->a;
	return Out;
}

static inline struct RGBAf_t RGBAf_Addi(const struct RGBAf_t *a, float b) 
{
	struct RGBAf_t Out;
	Out.r = a->r + b;
	Out.g = a->g + b;
	Out.b = a->b + b;
	Out.a = a->a + b;
	return Out;
}

/**************************************/

static inline struct RGBAf_t RGBAf_Sub(const struct RGBAf_t *a, const struct RGBAf_t *b) 
{
	struct RGBAf_t Out;
	Out.r = a->r - b->r;
	Out.g = a->g - b->g;
	Out.b = a->b - b->b;
	Out.a = a->a - b->a;
	return Out;
}

static inline struct RGBAf_t RGBAf_Subi(const struct RGBAf_t *a, float b) 
{
	struct RGBAf_t Out;
	Out.r = a->r - b;
	Out.g = a->g - b;
	Out.b = a->b - b;
	Out.a = a->a - b;
	return Out;
}

/**************************************/

static inline struct RGBAf_t RGBAf_Mul(const struct RGBAf_t *a, const struct RGBAf_t *b) 
{
	struct RGBAf_t Out;
	Out.r = a->r * b->r;
	Out.g = a->g * b->g;
	Out.b = a->b * b->b;
	Out.a = a->a * b->a;
	return Out;
}

static inline struct RGBAf_t RGBAf_Muli(const struct RGBAf_t *a, float b) 
{
	struct RGBAf_t Out;
	Out.r = a->r * b;
	Out.g = a->g * b;
	Out.b = a->b * b;
	Out.a = a->a * b;
	return Out;
}

/**************************************/

static inline struct RGBAf_t RGBAf_Div(const struct RGBAf_t *a, const struct RGBAf_t *b) 
{
	struct RGBAf_t Out;
	Out.r = a->r / b->r;
	Out.g = a->g / b->g;
	Out.b = a->b / b->b;
	Out.a = a->a / b->a;
	return Out;
}

static inline struct RGBAf_t RGBAf_DivSafe(const struct RGBAf_t *a, const struct RGBAf_t *b, const struct RGBAf_t *DivByZeroValue) 
{
	static const struct RGBAf_t Zero = {0,0,0,0};
	if(!DivByZeroValue) DivByZeroValue = &Zero;

	struct RGBAf_t Out;
	Out.r = (b->r == 0.0f) ? DivByZeroValue->r : (a->r / b->r);
	Out.g = (b->g == 0.0f) ? DivByZeroValue->g : (a->g / b->g);
	Out.b = (b->b == 0.0f) ? DivByZeroValue->b : (a->b / b->b);
	Out.a = (b->a == 0.0f) ? DivByZeroValue->a : (a->a / b->a);
	return Out;
}

static inline struct RGBAf_t RGBAf_Divi(const struct RGBAf_t *a, float b) 
{
#if 0
	struct RGBAf_t Out;
	Out.b = a->b / b;
	Out.g = a->g / b;
	Out.r = a->r / b;
	Out.a = a->a / b;
	return Out;
#else
	return RGBAf_Muli(a, 1.0f / b);
#endif
}

static inline struct RGBAf_t RGBAf_InvDivi(const struct RGBAf_t *a, float b) 
{
	struct RGBAf_t Out;
	Out.r = b / a->r;
	Out.g = b / a->g;
	Out.b = b / a->b;
	Out.a = b / a->a;
	return Out;
}

/**************************************/

static inline float RGBAf_Dot(const struct RGBAf_t *a, const struct RGBAf_t *b) 
{
	return a->b*b->b +
	       a->g*b->g +
	       a->r*b->r +
	       a->a*b->a ;
}

static inline struct RGBAf_t RGBAf_Sqrt(const struct RGBAf_t *x) 
{
	struct RGBAf_t Out;
	Out.r = sqrtf(x->r);
	Out.g = sqrtf(x->g);
	Out.b = sqrtf(x->b);
	Out.a = sqrtf(x->a);
	return Out;
}

static inline struct RGBAf_t RGBAf_Dist2(const struct RGBAf_t *a, const struct RGBAf_t *b) 
{
	struct RGBAf_t Out;
	Out = RGBAf_Sub(a, b);
	Out = RGBAf_Mul(&Out, &Out);
	return Out;
}

static inline struct RGBAf_t RGBAf_Dist(const struct RGBAf_t *a, const struct RGBAf_t *b) 
{
	struct RGBAf_t Out;
	Out = RGBAf_Dist2(a, b);
	Out = RGBAf_Sqrt(&Out);
	return Out;
}

static inline float RGBAf_AbsDist2(const struct RGBAf_t *a, const struct RGBAf_t *b) 
{
	struct RGBAf_t Out;
	Out = RGBAf_Sub(a, b);
	return RGBAf_Dot(&Out, &Out);
}

static inline float RGBAf_AbsDist(const struct RGBAf_t *a, const struct RGBAf_t *b) 
{
	return sqrtf(RGBAf_AbsDist2(a, b));
}

static inline float RGBAf_Len2(const struct RGBAf_t *x) 
{
	return RGBAf_Dot(x, x);
}

static inline float RGBAf_Len(const struct RGBAf_t *x) 
{
	return sqrtf(RGBAf_Len2(x));
}

/**************************************/

static inline struct RGBAf_t RGBAf_Abs(const struct RGBAf_t *x) 
{
	struct RGBAf_t Out;
	Out.r = (x->r < 0) ? (-x->r) : (x->r);
	Out.g = (x->g < 0) ? (-x->g) : (x->g);
	Out.b = (x->b < 0) ? (-x->b) : (x->b);
	Out.a = (x->a < 0) ? (-x->a) : (x->a);
	return Out;
}

/**************************************/

//! Colour distance function
static inline float RGBAf_ColDistance(const struct RGBAf_t *a, const struct RGBAf_t *b) 
{
	struct RGBAf_t d = RGBAf_Sub(a, b);
	return RGBAf_Len2(&d);
}

/**************************************/
//! EOF
/**************************************/
