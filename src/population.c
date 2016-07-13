#include "population.h"

#include <stdlib.h>
#include <stdint.h>
#include <strings.h>
#include <stdbool.h>

#include "network.h"

population_t *populationCreate( int numIndividuals, 
				uint64_t numInputs, uint64_t numLayers,
				network_layer_params_t *layerParams,
				bool createNets )
{
  int i;

  if( numIndividuals < 2 ) {
    return NULL;
  }

  // Object
  population_t *tmp = malloc( sizeof(population_t) );
  if( tmp == NULL ) {
    return NULL;
  }

  tmp->size = numIndividuals;

  // Networks
  tmp->elements = malloc( sizeof(population_element_t) * tmp->size );
  if( tmp->elements == NULL ) {
    free( tmp );
    return NULL;
  }

  // Initialise networks
  if( createNets ) {
    for( i = 0; i < numIndividuals; i++ ) {
      tmp->elements[i].network = networkCreate( numInputs, numLayers, layerParams, true );
    }
  }

  return tmp;
}

void populationReplaceIndividual( population_t *population, int individual, network_t *network )
{
  networkDestroy( population->elements[individual].network );
  population->elements[individual].network = network;
}

network_t *populationGetIndividual( population_t *population, int individual )
{
  return population->elements[individual].network;
}

static int compMin( const void *a, const void *b )
{
  population_element_t *p1 = (population_element_t*)a;
  population_element_t *p2 = (population_element_t*)b;

  // Sanitise values
  if( p1->score < 0 )
    return 1;
  if( p2->score < 0 )
    return -1;

  if( p1->score < p2->score )
    return -1;
  if( p1->score == p2->score )
    return 0;
  return 1;
}

static int compMax( const void *a, const void *b )
{
  population_element_t *p1 = (population_element_t*)a;
  population_element_t *p2 = (population_element_t*)b;

  // Sanitise values
  if( p1->score < 0 )
    return 1;
  if( p2->score < 0 )
    return -1;

  if( p1->score > p2->score )
    return -1;
  if( p1->score == p2->score )
    return 0;
  return 1;
}

population_t *populationSpawn( population_t *population, bool minimise )
{
#if 0
  int i, done;

  network_layer_params_t *layerParams = networkGetLayerParams( population->networks[0] );
  population_t *tmp = populationCreate( population->size,
					networkGetNumInputs( population->networks[0] ),
					networkGetNumLayers( population->networks[0] ),
					layerParams,
					false );
  free(layerParams);
  if( tmp == NULL ) {
    return NULL;
  }

  double min = 100000000;
  double max = 0;
  int best = 0;
  int worst = 0;

  double sumScore = 0;
  for( i = 0; i < population->size; i++ ) {
    sumScore += population->scores[i];

    if( (max < population->scores[i]) ||
	(max == population->scores[i] && rand() & 1) ) {
      max = population->scores[i];
      best = i;
    }

    if( (min > population->scores[i]) ||
	(min == population->scores[i] && rand() & 1) ) {
      min = population->scores[i];
      worst = i;
    }
  }

  // Let the best (or randomly chosen of the best) individual survive unchanged.
  if( minimise ) {
    tmp->networks[0] = networkCopy( population->networks[worst] );
  } else {
    tmp->networks[0] = networkCopy( population->networks[best] );
  }
  done = 1;

  for( ; done < tmp->size; done++ ) {
    network_t *mom, *dad;
    int momIdx, dadIdx;

    if( sumScore ) {
      // Pick random candidates with higher likelihood for better players
      i = 0;
      while( 1 ) {
	i = rand() % population->size;
	if( !minimise && population->scores[i % population->size] >= (rand() / (double)RAND_MAX) * sumScore ) {
	  mom = population->networks[i % population->size];
	  momIdx = i % population->size;
	  break;
	}
	if( minimise && population->scores[i % population->size] <= (rand() / (double)RAND_MAX) * sumScore ) {
	  mom = population->networks[i % population->size];
	  momIdx = i % population->size;
	  break;
	}
      }
      
      // Pick random candidates with higher likelihood for better players
      i = 0;
      while( 1 ) {
	i = rand() % population->size;
	if( (i != momIdx) &&
	    (population->scores[i % population->size] >= (rand() / (double)RAND_MAX) * sumScore) ) {
	  dad = population->networks[i % population->size];
	  break;
	}
	if( (i != momIdx) &&
	    minimise &&
	    (population->scores[i % population->size] <= (rand() / (double)RAND_MAX) * sumScore) ) {
	  dad = population->networks[i % population->size];
	  break;
	}
      }
    } else { // There are no scores so just pick random individuals
      momIdx = rand() % population->size;
      do {
	dadIdx = rand() % population->size;
      } while( dadIdx == momIdx );

      mom = population->networks[momIdx];
      dad = population->networks[dadIdx];
    }

    tmp->networks[done] = networkCombine( mom, dad );
    networkMutate( tmp->networks[done], 0.01 );
  }

  return tmp;
#else
  return NULL;
#endif
}

void populationRespawn( population_t *population, bool minimise )
{
  // Sort population according to score
  qsort( population->elements, population->size,
	 sizeof(population_element_t),
	 minimise ? compMin : compMax );

  // Keep top ten percent
  int numBest = population->size * 0.10;
  if( numBest < 1 ) numBest = 1;

  int done = numBest;

  // Keep an extra five percent randomly chosen from the rest
  int numRandom = population->size * 0.05;
  if( numRandom < 1 ) numRandom = 1;
  while( done < numBest + numRandom ) {
    int chosen = done + (rand() % (population->size - done));

    population_element_t tmp = population->elements[chosen];
    population->elements[chosen] = population->elements[done];
    population->elements[done] = tmp;

    done++;
  }

  // Fill rest of population with individuals spawned from the best and the randomly chosen
  for( ; done < population->size; done++ ) {
    int momIdx, dadIdx;

    momIdx = rand() % (numBest + numRandom);
    do {
      dadIdx = rand() % (numBest + numRandom);
    } while( dadIdx == momIdx );

    network_t *mom = population->elements[momIdx].network;
    network_t *dad = population->elements[dadIdx].network;

    // Cleanup
    networkDestroy( population->elements[done].network );

    // Magic happens here
    population->elements[done].network = networkCombine( mom, dad );
    networkMutate( population->elements[done].network, 0.01 );
  }
}

void populationClearScores( population_t *population )
{
  int i;
  for( i = 0; i < population->size; i++ ) {
    population->elements[i].score = 0;
  }
}

void populationSetScore( population_t *population, int individual, double score )
{
  population->elements[individual].score = score;
}

double populationGetScore( population_t *population, int individual )
{
  return population->elements[individual].score;
}

void populationDestroy( population_t *population )
{
  int i;
  for( i = 0; i < population->size; i++ ) {
    networkDestroy( population->elements[i].network );
  }
  free( population->elements );
  free( population );
}
