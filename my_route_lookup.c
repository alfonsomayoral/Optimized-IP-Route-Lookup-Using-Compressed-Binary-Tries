#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <sys/resource.h>
#include "io.h"
#include <stdbool.h>


#define OUTPUT_NAME ".out"

static int numeroNodos = 1;

// Estructura para representar un nodo del árbol binario
typedef struct Node {
    struct Node *left; // Puntero al hijo izquierdo
    struct Node *right; // Puntero al hijo derecho
    uint32_t prefix; // Prefijo de red
	char* prefix_bin; //Prefijo de red pasado a representación binaria en una cadena de caracteres.
    int prefixLength; // Longitud del prefijo
    int outInterface; // Interfaz de salida
	int bitID;	//índice del bit que se usa para decidir entre el hijo izquierdo y el derecho.
	bool isOut; //indicador de si el nodo está marcado como nodo de salida.

} Node;

//Funciones de Manejo del Árbol:
Node* createNode(uint32_t prefix, int prefixLength, int outInterface); // Crea un nuevo nodo del árbol con el prefijo, la longitud del prefijo y la interfaz de salida proporcionados.
void freeTree(Node *root); //Libera la memoria utilizada por todos los nodos del árbol.
char* prefixToBinaryString(uint32_t prefix, int prefixLength); //Convierte un prefijo de red en una cadena binaria

//Operaciones de Inserción y Compresión:
void insertNode(Node **root, uint32_t prefix, int prefixLength, int outInterface); //Inserta un nodo en el árbol según el prefijo proporcionado. El árbol se construye de manera que los prefijos más largos estén más cerca de la raíz.
void compressTree(Node **root); //Comprime el árbol buscando nodos que puedan fusionarse o eliminarse según ciertos criterios, como tener un único hijo y compartir la misma interfaz de 		salida.
//Funciones de Búsqueda:
int searchInterface(Node *root, uint32_t ipAddress, int *accessesNodes); //Busca la interfaz de salida correspondiente a una dirección IP en el árbol. Utiliza el árbol comprimido para buscar eficientemente la interfaz asociada al prefijo con el mayor número de bits coincidentes con la dirección IP.

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Uso: %s FIB InputPacketFile\n", argv[0]);
        return 1;
    }

    char *fibFileName = argv[1];
    char *inputFileName = argv[2];

    int result = initializeIO(fibFileName, inputFileName);
    if (result != OK) {
        printIOExplanationError(result);
        return 1;
    }

    // Crear el árbol binario de búsqueda (Patricia trie)
    Node *root = NULL;

    // Leer la tabla de enrutamiento (FIB) y construir el árbol
    uint32_t prefix;
    int prefixLength, outInterface;
    int numAccesses = 0;

    while (readFIBLine(&prefix, &prefixLength, &outInterface) == OK) {
        insertNode(&root, prefix, prefixLength, outInterface);
    }
	printf("numero de nodos = %d\n", numeroNodos);
	compressTree(&root);
	printf("numero de nodos = %d\n", numeroNodos);
    // Procesar los paquetes de entrada
    FILE *inputFile = fopen(inputFileName, "r");
    if (inputFile == NULL) {
        printf("Error al abrir el archivo de paquetes de entrada.\n");
        freeTree(root);
        return 1;
    }

    char outputFileName[100];
    sprintf(outputFileName, "%s%s", inputFileName, OUTPUT_NAME);
    FILE *outputFile = fopen(outputFileName, "w");
    if (outputFile == NULL) {
        printf("Error al crear el archivo de salida.\n");
        freeTree(root);
        fclose(inputFile);
        return 1;
    }

    int packetsProcessed = 0;
    int totalNodeAccesses = 0;
    double totalComputationTime = 0;

    uint32_t ipAddress;

    while (readInputPacketFileLine(&ipAddress) == OK) {
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC_RAW, &start);

        int outInterface = searchInterface(root, ipAddress, &numAccesses);

        clock_gettime(CLOCK_MONOTONIC_RAW, &end);

        double computationTime = (end.tv_sec - start.tv_sec) * 1e9 + (end.tv_nsec - start.tv_nsec);
        printOutputLine(ipAddress, outInterface, &start, &end, &computationTime, numAccesses);

        packetsProcessed++;
        totalNodeAccesses += numAccesses; // Se suma el acceso al nodo raíz
    }

    fclose(inputFile);
    fclose(outputFile);
    freeTree(root);

    // Calcular estadísticas y escribir el resumen en el archivo de salida
    double averageNodeAccesses = (double)totalNodeAccesses / packetsProcessed;
    double averageComputationTime = totalComputationTime / packetsProcessed;
		
	printSummary(packetsProcessed, averageNodeAccesses, averageComputationTime);

    return 0;
}

// Función para crear un nuevo nodo del árbol
Node* createNode(uint32_t prefix, int prefixLength, int outInterface) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    if (newNode == NULL) {
        printf("Error al asignar memoria para el nodo del árbol.\n");
        exit(1);
    }
    newNode->left = NULL;
    newNode->right = NULL;
    newNode->prefix = prefix;
	newNode->prefix_bin = prefixToBinaryString(prefix, prefixLength);
    newNode->prefixLength = prefixLength;
    newNode->outInterface = outInterface;
	newNode->isOut=false;
	newNode->bitID=32;
    return newNode;
}

void freeTree(Node *root) {
    if (root != NULL) {
        freeTree(root->left);
        freeTree(root->right);
        free(root);
    }
}

char* prefixToBinaryString(uint32_t prefix, int prefixLength) {
    char* binaryPrefix = (char*)malloc(33 * sizeof(char)); // 32 bits + '\0'
    if (binaryPrefix == NULL) {
        printf("Error al asignar memoria para la representación binaria del prefijo.\n");
        exit(1);
    }
    binaryPrefix[32] = '\0'; // Terminador de cadena

    for (int i = 0; i < 32; i++) {
        if (i < prefixLength) {
            // Extraer el i-ésimo bit del prefijo y agregarlo a la cadena binaria
            uint32_t bit = (prefix >> (31 - i)) & 1;
            binaryPrefix[i] = bit + '0';
        } else {
            // Rellenar con ceros si la longitud del prefijo es menor que 32 bits
            binaryPrefix[i] = '0';
        }
    }
    return binaryPrefix;
}

// Función para insertar una ruta en el Trie Patricia
void insertNode(Node **root, uint32_t prefix, int prefixLength, int outInterface) {
    if (*root == NULL) {
        // Creamos un nuevo nodo y lo asignamos como raíz
        *root = createNode(prefix, prefixLength, outInterface);
        return;
    }

    Node *current = *root;

    for (int i = 0; i < prefixLength; i++) {
        int bit = (prefix >> (32 - i - 1)) & 1;
        current->bitID=i;
        if (bit == 0) {
            if (current->left == NULL) {
                current->left = createNode(prefix, prefixLength, outInterface);
                numeroNodos++;
            }
            current = current->left;
        } else if (bit == 1) {
            if (current->right == NULL) {
                current->right = createNode(prefix, prefixLength, outInterface); 
                numeroNodos++;
            }
            current = current->right;
        }
    }
}

// Función para comprimir el árbol trie
void compressTree(Node **root) {
    Node *temp = *root;

    if (temp == NULL) {
        return; // Si el nodo es nulo, no hay nada que comprimir, así que se retorna
    }

    // Si el nodo tiene un hijo izquierdo único y su interfaz es 0, se comprime
    if (temp->left != NULL && temp->right == NULL && temp->isOut==false) {
        Node *child_left = temp->left;
		if((child_left->left == NULL && child_left->right != NULL)| (child_left->left != NULL && child_left->right == NULL)){
			child_left->bitID=temp->bitID;
		}//si solo tiene un nieto
        free(temp); // Liberar el nodo current
        *root = child_left; // Asignar el hijo izquierdo como nueva raíz
        compressTree(root); // Llamar recursivamente para comprimir más nodos
		numeroNodos--;
    } 
    // Si el nodo tiene un hijo derecho único y su interfaz es 0, se comprime
    else if (temp->left == NULL && temp->right != NULL && temp->isOut==false) {
        Node *child_right = temp->right;
		if((child_right->left == NULL && child_right->right != NULL)| (child_right->left != NULL && child_right->right == NULL)){
			child_right->bitID=temp->bitID;
		}
        free(temp); // Liberar el nodo current
        *root = child_right; // Asignar el hijo derecho como nueva raíz
        compressTree(root); // Llamar recursivamente para comprimir más nodos
		numeroNodos--;
    } 
    // Si el nodo no es comprimible, se llama recursivamente a la función para los subárboles izquierdo y derecho
    else {
        compressTree(&(temp->left));
        compressTree(&(temp->right));
    }
}



bool isOut;

// Función para buscar la interfaz de salida correspondiente a una dirección IP
int searchInterface(Node *root, uint32_t ipAddress, int *accessesNodes) {
    if (root == NULL) {
        return 0; // Si el árbol está vacío, devolvemos -1 indicando que no se encontró ninguna interfaz
    }

    // Incrementamos el contador de accesos a nodos
    
	*accessesNodes = 0;
    // Variable para almacenar la interfaz asociada al prefijo con mayor número de bits coincidentes
    int maxInterface = 0;

    Node *current = root;
    int bitIndex = current->bitID;


    // Iteramos sobre los bits de la dirección IP
    while (current != NULL && bitIndex < 32) {
        int bit = (ipAddress >> (31 - bitIndex)) & 1; // Obtener el i-ésimo bit de la dirección IP

        // Verificar si el nodo current tiene un prefijo asignado y si coincide con la IP
        if (current->prefixLength > 0 && (current->prefix >> (32 - current->prefixLength)) == (ipAddress >> (32 - current->prefixLength))) {
            // currentizar la interfaz asociada al prefijo con el mayor número de bits coincidentes
            if (current->outInterface > maxInterface) {
                maxInterface = current->outInterface;
            }
        }

        // Mover al siguiente nodo
        if (bit == 0) {
            current = current->left;
			bitIndex = current->bitID -1;
		    if (current->prefixLength > 0 && (current->prefix >> (32 - current->prefixLength)) == (ipAddress >> (32 - current->prefixLength))) {
		        // currentizar la interfaz asociada al prefijo con el mayor número de bits coincidentes
		        if (current->outInterface > maxInterface) {
		            maxInterface = current->outInterface;
		        }
		    }
			(*accessesNodes)++;
        } else {
            current = current->right;
			bitIndex = current->bitID -1;
		    if (current->prefixLength > 0 && (current->prefix >> (32 - current->prefixLength)) == (ipAddress >> (32 - current->prefixLength))) {
		        // currentizar la interfaz asociada al prefijo con el mayor número de bits coincidentes
		        if (current->outInterface > maxInterface) {
		            maxInterface = current->outInterface;
		        }
		    }
			(*accessesNodes)++;
        }

        bitIndex++;
    }

    return maxInterface; // Devolver la interfaz asociada al prefijo con el mayor número de bits coincidentes
}



