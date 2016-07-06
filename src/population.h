#ifndef POPULATION_H
#define POPULATION_H

#include <stdint.h>

#include "network.h"

typedef struct population_s {
  int         size;
  network_t **networks;
  int32_t    *scores;
} population_t;


// Create an entirely new population of networks,
population_t *populationCreate( int numIndividuals,
				uint64_t numInputs, uint64_t numLayers,
				network_layer_params_t *layerParams,
				bool createNets );

// Replace a network, assume it's has the same characteristics as the others
void populationReplaceIndividual( population_t *population, int individual, network_t *network );

// Spawn a new population from the best individuals of the last one.
population_t *populationSpawn( population_t *population );

// Set all scores to 0.
void populationClearScores( population_t *population );

// Set a score.
void populationSetScore( population_t *population, int individual, float score );

// Clear all memory of a population and destroy the associated networks.
void populationDestroy( population_t *population );

#endif
