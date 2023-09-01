/**************************************/
#pragma once
/**************************************/
#include "Colourspace.h"
/**************************************/

struct QuantCluster_t 
{
	int             Next;
	int             nPoints;
	float           TrainWeight;
	float           DistWeight;
	struct RGBAf_t  Train;
	struct RGBAf_t  Dist;
	struct RGBAf_t  Centroid;
};

/**************************************/

//! Perform total vector quantization
void QuantCluster_Quantize(struct QuantCluster_t *Clusters, int nCluster, const struct RGBAf_t *Data, int nData, int32_t *DataClusters, int nPasses);

/**************************************/
//! EOF
/**************************************/
