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
  tmp->networks = malloc( sizeof(network_t*) * tmp->size );
  if( tmp->networks == NULL ) {
    free( tmp );
    return NULL;
  }

  // Initialise networks
  if( createNets ) {
    for( i = 0; i < numIndividuals; i++ ) {
      tmp->networks[i] = networkCreate( numInputs, numLayers, layerParams, true );
    }
  }

  // Scores
  tmp->scores = malloc( sizeof(int32_t) * tmp->size );
  if( tmp->scores == NULL ) {
    free( tmp->networks );
    free( tmp );
    return NULL;
  }

  return tmp;
}

population_t *populationSpawn( population_t *population )
{
  int i, done;

  population_t *tmp = populationCreate( population->size,
					networkGetNumInputs( population->networks[0] ),
					networkGetNumLayers( population->networks[0] ),
					networkGetLayerParams( population->networks[0] ),
					false );
  if( tmp == NULL ) {
    return NULL;
  }

  int max = 0;
  int best = 0;

  uint64_t sumScore = 0;
  for( i = 0; i < population->size; i++ ) {
    sumScore += population->scores[i];

    if( (max < population->scores[i]) ||
	(max == population->scores[i] && rand() & 1) ) {
      max = population->scores[i];
      best = i;
    }
  }

  // Let the best (or randomly chosen of the best) individual survive unchanged.
  tmp->networks[0] = networkCopy( population->networks[best] );
  done = 1;

  for( ; done < tmp->size; done++ ) {
    network_t *mom, *dad;
    int momIdx, dadIdx;

    if( sumScore ) {
      // Pick random candidates with higher likelihood for better players
      i = 0;
      while( 1 ) {
	i = rand() % population->size;
	if( population->scores[i % population->size] >= rand() % sumScore ) {
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
	    (population->scores[i % population->size] >= rand() % sumScore) ) {
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
    networkMutate( tmp->networks[done] );
  }

  return tmp;
}

void populationClearScores( population_t *population )
{
  int i;
  for( i = 0; i < population->size; i++ ) {
    population->scores[i] = 0;
  }
}

void populationSetScore( population_t *population, int individual, float score )
{
  population->scores[individual] = score;
}

void populationDestroy( population_t *population )
{
  int i;
  free( population->scores );
  for( i = 0; i < population->size; i++ ) {
    networkDestroy( population->networks[i] );
  }
  free( population->networks );
  free( population );
}
