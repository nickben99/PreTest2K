// PreTest.c
#include <stdio.h>
#include <stdlib.h>

typedef int bool;
enum { false, true };

/////////////////// input data /////////////////////////////////////////////////////////////////////////////////////

struct Data
{
	int numCities;
	int* mAdjacencyTable;
};

// the adjacency table is undirected so only needs to be large enough to accomodate the lower triangular portion
// of it, this function converts the row and col index to an index which can be used in the adjacency table
int ConvertRowAndColToAdjacencyTableIndex(int row, int col)
{
	if (row == col)
	{
		return -1;
	}
	if (col > row)
	{
		int temp = col;
		col = row;
		row = temp;
	}

	// this if statement could be removed and the return statements be made more complicated
	if (row%2 == 1) // odd row
	{
		return (row*(row/2)) + col;
	}
	// even row
	{
		int tempRow = row - 1;
		return (tempRow*(tempRow/2)) + tempRow + col;
	}
}

bool AllocateAdjacencyTableData(struct Data* pOutput)
{
	if (pOutput)
	{
		if (pOutput->numCities <= 0)
		{
			return false;
		}
		// ConvertRowAndColToAdjacencyTableIndex tells us how much data is required for the lower left triangle of the 
		// adjacency matrix
		pOutput->mAdjacencyTable = (int*)malloc( ConvertRowAndColToAdjacencyTableIndex(pOutput->numCities, 0)*sizeof(int) );
		if (pOutput->mAdjacencyTable)
		{
			return true;
		}
	}
	return false;
}

void FreeAdjacencyTableData(struct Data* pOutput)
{
	if (pOutput && pOutput->mAdjacencyTable)
	{
		free( pOutput->mAdjacencyTable);
	}
}

///////////////////// file i/o //////////////////////////////////////////////////////////////////////////////////////

enum { fileError=-3, endOfFile, blockedPath };
int CalculateNextValue(FILE* file)
{
	if (file)
	{
		char character = 0;
		int integer = 0;
	  
		while (!feof(file))
		{
			character = (char)getc(file);
			if (character >= '0' && character <= '9') // it's an integer
			{
				fseek( file , -1 , SEEK_CUR ); // go back to the start of the integer
				fscanf_s(file, "%d", &integer); // get the integer
				return integer;
			}
			else if (character == 'x' || character == 'X') // indication of blocked path
			{
				return blockedPath;
			} 
	    }
		return endOfFile; 
	}
	return fileError;
}

bool ParseDataFile(struct Data* pOutput, char* pDataInputFile)
{
	bool isSuccess = false;
	FILE* file = 0;
	if (0 != fopen_s(&file, pDataInputFile, "r"))
	{
		printf("No input file specified");
		return isSuccess; // error, could not open file
	}

	pOutput->numCities = CalculateNextValue(file); 	
	if (pOutput->numCities > 0)
	{
		int row = 0;
		int col = 0;
		
		if (AllocateAdjacencyTableData(pOutput))
		{
			isSuccess = true;
			for (; row < pOutput->numCities; ++row)
			{
				for (col = 0; col < pOutput->numCities; ++col)
				{
					int adjacencyTableIndex = 0;
					if (row <= col)
					{
						break;
					}

					adjacencyTableIndex = ConvertRowAndColToAdjacencyTableIndex(row, col);
					pOutput->mAdjacencyTable[adjacencyTableIndex] = CalculateNextValue(file);
					if (endOfFile == pOutput->mAdjacencyTable[adjacencyTableIndex] || 
						fileError == pOutput->mAdjacencyTable[adjacencyTableIndex])
					{ // error
						isSuccess = false;
					}					
				}
			}
			if (!isSuccess)
			{
				printf("Not enough values specified in data file");
			}
		}
		else
		{
			printf("Data allocation error");
		}
	}
	fclose(file);
	return isSuccess;
}

/////////////////////////////////// path node //////////////////////////////////////////////////////////////////

struct PathNode
{
	int mCity; // city node index
	int mTime; // how long to get there
};

/////////////////////////////////// min heap ///////////////////////////////////////////////////////////////////

struct MinHeap
{	
	int numElements;
	int maxElements;
	struct PathNode* elements;
};

bool MinHeapInit(struct MinHeap* pHeap, int maxElements)
{
	if (pHeap)
	{
		pHeap->numElements = 0;
		pHeap->maxElements = maxElements;
		pHeap->elements = 0;
		if (pHeap->maxElements > 0)
		{
			pHeap->elements = (struct PathNode*)malloc(maxElements*sizeof(struct PathNode));
			if (0 == pHeap->elements)
			{
				return false;
			}			
		}
		return true;
	}
	return false;
}

void MinHeapDestroy(struct MinHeap* pHeap)
{
	if (pHeap && pHeap->elements)
	{
		free(pHeap->elements);
	}
}

void MinHeapBubbleDown(struct MinHeap* pHeap, int index)
{
	if (pHeap && index >= 0 && index < pHeap->numElements)
	{
		int childIndexLeft = (index*2)+1;
		int childIndexRight = childIndexLeft + 1;

		while (childIndexLeft < pHeap->numElements)
		{
			int smallestChild = childIndexRight;
			if (childIndexRight >= pHeap->numElements)
			{
				smallestChild = childIndexLeft;
			}
			else if (pHeap->elements[childIndexLeft].mTime < pHeap->elements[childIndexRight].mTime)
			{
				smallestChild = childIndexLeft;
			}

			if (pHeap->elements[smallestChild].mTime < pHeap->elements[index].mTime)
			{
				struct PathNode temp = pHeap->elements[index];
				pHeap->elements[index] = pHeap->elements[smallestChild];
				pHeap->elements[smallestChild] = temp;
			}
			else
			{
				break;
			}

			index = smallestChild;			
			childIndexLeft = (index*2)+1;
			childIndexRight = childIndexLeft + 1;
		}
	}
}

void MinHeapBubbleUp(struct MinHeap* pHeap, int index)
{
	if (pHeap && index < pHeap->numElements)
	{
		int parentIndex = (index-1)/2;

		while (index > 0 && pHeap->elements[index].mTime < pHeap->elements[parentIndex].mTime)
		{
			struct PathNode temp = pHeap->elements[index];
			pHeap->elements[index] = pHeap->elements[parentIndex];
			pHeap->elements[parentIndex] = temp;

			index = parentIndex;
			parentIndex = (index-1)/2;
		}
	}
}

void MinHeapAddTo(struct MinHeap* pHeap, struct PathNode node)
{
	if (pHeap && pHeap->numElements < pHeap->maxElements)
	{
		pHeap->elements[pHeap->numElements++] = node;
		MinHeapBubbleUp(pHeap, pHeap->numElements-1);
	}
}

void MinHeapPop(struct MinHeap* pHeap, struct PathNode* pOutNode)
{
	if (pHeap && pHeap->numElements > 0)
	{
		*pOutNode = pHeap->elements[0];
		pHeap->elements[0] = pHeap->elements[--pHeap->numElements];
		MinHeapBubbleDown(pHeap, 0);
	}
}

int MinHeapFind(struct MinHeap* pHeap, struct PathNode node)
{
	int index = 0;
	for (; index < pHeap->numElements; ++index)
	{
		if (pHeap->elements[index].mCity == node.mCity)
		{
			return index;
		}
	}
	return -1;
}

void MinHeapChangeNodeWeight(struct MinHeap* pHeap, int index, int newWeight)
{
	if (pHeap && index >= 0 && index < pHeap->numElements && newWeight != pHeap->elements[index].mTime)
	{
		if (newWeight < pHeap->elements[index].mTime) // bubble up
		{
			pHeap->elements[index].mTime = newWeight;
			MinHeapBubbleUp(pHeap, index);
		}
		else // bubble down
		{
			pHeap->elements[index].mTime = newWeight;
			MinHeapBubbleDown(pHeap, index);
		}
	}
}

////////////////////////////////// array ///////////////////////////////////////////////////////////////////////

struct NodeArray
{
	int numElements;
	int maxElements;
	struct PathNode* elements;
};

bool NodeArrayInit(struct NodeArray* pArray, int maxElements)
{
	if (pArray)
	{
		pArray->numElements = 0;
		pArray->maxElements = maxElements;
		pArray->elements = 0;
		if (pArray->maxElements > 0)
		{
			pArray->elements = (struct PathNode*)malloc(maxElements*sizeof(struct PathNode));
			if (0 == pArray->elements)
			{
				return false;
			}			
		}
		return true;
	}
	return false;
}

void NodeArrayDestroy(struct NodeArray* pArray)
{
	if (pArray && pArray->elements)
	{
		free(pArray->elements);
	}
}

void NodeArrayAddTo(struct NodeArray* pArray, struct PathNode node)
{
	if (pArray && pArray->numElements < pArray->maxElements)
	{
		pArray->elements[pArray->numElements++] = node;
	}
}

int NodeArrayFind(struct NodeArray* pArray, struct PathNode node)
{
	int index = 0;
	for (; index < pArray->numElements; ++index)
	{
		if (pArray->elements[index].mCity == node.mCity)
		{
			return index;
		}
	}
	return -1;
}

/////////////////////////////////// dijkstra algorithm ///////////////////////////////////////////////////////////

bool DijkstraSearch(struct Data* pOutput)
{
	struct MinHeap openList;
	struct NodeArray closedList;
	bool success = false;

	if (MinHeapInit(&openList, pOutput->numCities) && NodeArrayInit(&closedList, pOutput->numCities))
	{
		struct PathNode startingNode;
		startingNode.mCity = 0;
		startingNode.mTime = 0;
		MinHeapAddTo(&openList, startingNode);

		while (openList.numElements > 0)
		{
			int city = 0;
			struct PathNode lowestNode; 
			MinHeapPop(&openList, &lowestNode); // get lowest from open list
			NodeArrayAddTo(&closedList, lowestNode); // add lowest to closed list

			for (; city < pOutput->numCities; ++city)
			{
				struct PathNode potentialNode;
				if (lowestNode.mCity == city)
				{
					continue; // don't check travelling to the city we're at
				}
				
				potentialNode.mCity = city;				
				if (NodeArrayFind(&closedList, potentialNode) >= 0)
				{
					continue; // it's already on the closed list, not interested
				}

				{
					int openListIndex = MinHeapFind(&openList, potentialNode);
					int moveTime = pOutput->mAdjacencyTable[ ConvertRowAndColToAdjacencyTableIndex(lowestNode.mCity, city) ];
					if (moveTime >= 0)
					{
						if (openListIndex != -1) // it's already on the open list, try and modify time value
						{
							if ( openList.elements[openListIndex].mTime > (lowestNode.mTime + moveTime) )
							{
								MinHeapChangeNodeWeight(&openList, openListIndex, lowestNode.mTime + moveTime);
							}
						}
						else // add to open list
						{
							potentialNode.mTime = (lowestNode.mTime + moveTime);
							MinHeapAddTo(&openList, potentialNode);
						}
					}
				}
			}
		}
		if (closedList.numElements == closedList.maxElements) // have we visited all the cities
		{
			int longestTime = 0;
			int index = 0;
			for (; index < closedList.numElements; ++index)
			{		
				if (closedList.elements[index].mTime > longestTime)
				{
					longestTime = closedList.elements[index].mTime;
				}
			}
			printf("The time taken for the message to reach all cities is %d", longestTime);
		}
		else
		{
			printf("No path found");
		}
		success = true;
	}		
	if (!success)
	{
		printf("Data allocation error");
	}	

	NodeArrayDestroy(&closedList);
	MinHeapDestroy(&openList);
	return success;
}

int main(int argc, char* argv[])
{
	struct Data inputData;
	char* pDataInputFile = 0;
	int error = false;
	
	if (argc > 1)
	{
		inputData.mAdjacencyTable = 0;
		pDataInputFile = argv[1]; // pass data input file as first param to prog
		error = true;
		if (ParseDataFile(&inputData, pDataInputFile) && DijkstraSearch(&inputData))
		{			
			error = false;				
		}	
		FreeAdjacencyTableData(&inputData);
	}
	else
	{
		printf("No input file specified");
		error = true;
	}
	
	printf("\n\nPress enter key to exit...");
	getchar();	
	return error;
}

