#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <windows.h>
#include <time.h>
#include <string.h>
#include <signal.h>

#define MAX_LINE 256

HANDLE mutexList, mutexOutput, mutexVariables;
HANDLE semaphoreSlots, semaphoreItems;
HANDLE* threadHWorker = NULL;
HANDLE threadHMonitor = NULL;

FILE* logFile = NULL;
FILE* file = NULL;

typedef struct Order {
	int id;
	char customerName[50];
	int quantidade;
	float precoTotal;
	struct Order* next;
} Order;

Order* head = NULL;
Order* tail = NULL;

int countPending = 0;
int countProcessing = 0;
int countProcessed = 0;

int minWorkTime = 0;
int maxWorkTime = 0;
int nWorkers = 0;

int finished = 0;
int closing = 0;

void pushOrder(Order* newOrder) {
	newOrder->next = NULL;

	WaitForSingleObject(semaphoreSlots, INFINITE);
	WaitForSingleObject(mutexList, INFINITE);

	if (head == NULL) {
		head = newOrder;
		tail = newOrder;
	}
	else {
		tail->next = newOrder;
		tail = newOrder;
	}

	countPending++;

	ReleaseMutex(mutexList);
	ReleaseSemaphore(semaphoreItems, 1, NULL);
}

Order* popOrder() {
	WaitForSingleObject(semaphoreItems, INFINITE);
	WaitForSingleObject(mutexList, INFINITE);

	if (head == NULL) {
		ReleaseMutex(mutexList);
		return NULL;
	}

	Order* order = head;
	head = head->next;

	if (head == NULL) {
		tail = NULL;
	}

	countPending--;

	ReleaseMutex(mutexList);
	ReleaseSemaphore(semaphoreSlots, 1, NULL);

	return order;
}

DWORD WINAPI worker(LPVOID params)
{
	int myID = *(int*)params;
	free(params);

	srand((unsigned int)time(NULL) ^ (myID + 1));

	WaitForSingleObject(mutexOutput, INFINITE);
	printf("[Worker %d] Iniciado e a aguardar tarefas...\n", myID);
	ReleaseMutex(mutexOutput);

	while (1) {
		Order* myOrder = popOrder();

		if (closing) {
			if (myOrder != NULL) {
				free(myOrder);
			}
			break;
		}

		if (myOrder == NULL) {
			if (finished) {
				break;
			}
			continue;
		}

		WaitForSingleObject(mutexOutput, INFINITE);
		printf("[Worker %d] RECEBEU encomenda %d (Cliente: %s). A preparar...\n",
			myID, myOrder->id, myOrder->customerName);
		ReleaseMutex(mutexOutput);

		WaitForSingleObject(mutexVariables, INFINITE);
		countProcessing++;
		int waitTime = minWorkTime + (rand() % (maxWorkTime - minWorkTime + 1));
		ReleaseMutex(mutexVariables);

		WaitForSingleObject(mutexOutput, INFINITE);
		printf("[Worker %d] A PROCESSAR encomenda %d... (Tempo estimado: %dms)\n",
			myID, myOrder->id, waitTime);
		ReleaseMutex(mutexOutput);

		Sleep(waitTime);

		WaitForSingleObject(mutexOutput, INFINITE);
		printf("[Worker %d] CONCLUIU encomenda %d (Cliente: %s)\n",
			myID, myOrder->id, myOrder->customerName);

		if (logFile) {
			if (!closing) {
				fprintf(logFile, "Worker %d processou a encomenda %d (Cliente: %s)\n",
					myID, myOrder->id, myOrder->customerName);
				fflush(logFile);
			}
			else {
				fprintf(logFile, "Worker %d [ENCERRAMENTO FORCADO] concluiu a encomenda %d (Cliente: %s)\n",
					myID, myOrder->id, myOrder->customerName);
				fflush(logFile);
			}
		}
		ReleaseMutex(mutexOutput);

		WaitForSingleObject(mutexVariables, INFINITE);
		countProcessing--;
		countProcessed++;
		ReleaseMutex(mutexVariables);

		free(myOrder);
	}

	WaitForSingleObject(mutexOutput, INFINITE);
	printf("[Worker %d] A encerrar.\n", myID);
	ReleaseMutex(mutexOutput);

	return 0;
}

DWORD WINAPI monitor(LPVOID params)
{
	int pend, proc, concl;

	while (1) {

		WaitForSingleObject(mutexList, INFINITE);
		pend = countPending;
		ReleaseMutex(mutexList);

		WaitForSingleObject(mutexVariables, INFINITE);
		proc = countProcessing;
		concl = countProcessed;

		if (finished && pend == 0 && proc == 0) {

			printf("[MONITOR] Pendentes: %03d | A Processar: %02d | Concluidos: %04d\n",
				pend, proc, concl);

			ReleaseMutex(mutexVariables);
			break;
		}

		ReleaseMutex(mutexVariables);

		printf("[MONITOR] Pendentes: %03d | A Processar: %02d | Concluidos: %04d\n",
			pend, proc, concl);

		Sleep(1000);
	}
	return 0;
}

void CloseHandling(int signal_number)
{
	printf("\n[SIGNAL] Encerramento iniciado (SIGINT)\n");

	WaitForSingleObject(mutexOutput, INFINITE);

	if (logFile) {
		fprintf(logFile, "ATENCAO: FOI GERADA UMA INTERRUPCAO!!!\n");
		fflush(logFile);
	}
	ReleaseMutex(mutexOutput);

	closing = 1;
	finished = 1;

	WaitForSingleObject(mutexList, INFINITE);

	Order* curr = head;
	while (curr) {
		Order* tmp = curr;
		curr = curr->next;
		free(tmp);
	}
	head = NULL;
	tail = NULL;
	countPending = 0;

	ReleaseMutex(mutexList);
	printf("[SIGNAL] Lista de pendentes limpa.\n");

	for (int i = 0; i < nWorkers; i++) {
		ReleaseSemaphore(semaphoreItems, 1, NULL);
	}

	if (threadHWorker != NULL) {
		for (int i = 0; i < nWorkers; i++) {
			if (threadHWorker[i] != NULL) {
				WaitForSingleObject(threadHWorker[i], INFINITE);
				CloseHandle(threadHWorker[i]);
			}
		}
	}

	if (threadHMonitor != NULL) {
		WaitForSingleObject(threadHMonitor, INFINITE);
		CloseHandle(threadHMonitor);
	}

	if (file != NULL) {
		fclose(file);
		file = NULL;
	}

	if (logFile != NULL) {
		fclose(logFile);
		logFile = NULL;
	}

	if (mutexList) CloseHandle(mutexList);
	if (mutexOutput) CloseHandle(mutexOutput);
	if (mutexVariables) CloseHandle(mutexVariables);
	if (semaphoreSlots) CloseHandle(semaphoreSlots);
	if (semaphoreItems) CloseHandle(semaphoreItems);

	printf("[CLOSE] Recursos libertados com sucesso.\n");
	exit(0);
}

void mainErrorHandeling(int argc, char* argv[]) {
	if (argc != 6) {
		printf("Numero incorreto de argumentos!\n");
		printf("Sintaxe: <Filename> <NWorkers> <MaxOrders> <MinWorkTime> <MaxWorkTime>\n");
		exit(1);
	}

	if (atoi(argv[2]) <= 0) {
		printf("Erro: Numero de workers invalido.\n");
		exit(1);
	}

	if (atoi(argv[3]) <= 0) {
		printf("Erro: Numero maximo de ordens invalido.\n");
		exit(1);
	}

	if (atoi(argv[2]) > atoi(argv[3])) {
		printf("ATENCAO: Numero de trabalhadores maior que bloco de encomendas!!");
	}

	minWorkTime = atoi(argv[4]);
	maxWorkTime = atoi(argv[5]);

	if (minWorkTime <= 0) {
		printf("Erro: Duracao minima invalida.\n");
		exit(1);
	}

	if (maxWorkTime <= 0) {
		printf("Erro: Duracao maxima invalida.\n");
		exit(1);
	}

	if (maxWorkTime < minWorkTime) {
		printf("Erro: Duracao maxima (%d) nao pode ser menor que a minima (%d).\n", maxWorkTime, minWorkTime);
		exit(1);
	}
}

int main(int argc, char* argv[])
{

	mainErrorHandeling(argc, argv);

	char* filePath = argv[1];
	nWorkers = atoi(argv[2]);
	int maxOrders = atoi(argv[3]);

	logFile = fopen("output.log", "w");
	if (!logFile) {
		printf("Erro ao criar output.log\n");
		return 1;
	}

	file = fopen(filePath, "r");
	if (!file) {
		printf("Erro a abrir ficheiro: %s\n", filePath);
		return 1;
	}

	DWORD* threadIDWorker = (DWORD*)malloc(sizeof(DWORD) * nWorkers);
	DWORD threadIDMonitor;
	threadHWorker = (HANDLE*)malloc(sizeof(HANDLE) * nWorkers);

	mutexList = CreateMutex(NULL, FALSE, NULL);
	if (mutexList == NULL) {
		printf("Create list mutex failed!\n");
		exit(1);
	}

	mutexOutput = CreateMutex(NULL, FALSE, NULL);
	if (mutexOutput == NULL) {
		printf("Create output mutex failed!\n");
		exit(1);
	};

	mutexVariables = CreateMutex(NULL, FALSE, NULL);
	if (mutexVariables == NULL) {
		printf("Create variables mutex failed!\n");
		exit(1);
	};

	semaphoreSlots = CreateSemaphore(NULL, maxOrders, maxOrders, NULL);
	if (semaphoreSlots == NULL) {
		printf("Create read semaphore failed!\n");
		exit(1);
	};

	semaphoreItems = CreateSemaphore(NULL, 0, maxOrders, NULL);
	if (semaphoreItems == NULL) {
		printf("Create write semaphore failed!\n");
		exit(1);
	};

	threadHMonitor = CreateThread(NULL, 0, monitor, NULL, 0, &threadIDMonitor);
	if (threadHMonitor == NULL) return 0;

	for (int i = 0; i < nWorkers; i++) {

		int* pID = (int*)malloc(sizeof(int));
		*pID = i;

		threadHWorker[i] = CreateThread(NULL, 0, worker, (LPVOID)pID, 0, &threadIDWorker[i]);
		if (threadHWorker[i] == NULL) {
			printf("Erro ao criar thread worker %d\n", i);
			free(pID); 
			exit(1);
		}
	}

	int idTemp;
	char nameTemp[50];
	int qtdTemp;
	float priceTemp;

	while (fscanf(file, "%d;%49[^;];%d;%f", &idTemp, nameTemp, &qtdTemp, &priceTemp) == 4)
	{
		signal(SIGINT, CloseHandling);

		Order* newOrder = (Order*)malloc(sizeof(Order));
		if (newOrder == NULL) {
			printf("Erro de memoria\n");
			break;
		}

		newOrder->id = idTemp;
		strcpy(newOrder->customerName, nameTemp);
		newOrder->quantidade = qtdTemp;
		newOrder->precoTotal = priceTemp;

		pushOrder(newOrder);
	}

	fclose(file);

	finished = 1;

	for (int i = 0; i < nWorkers; i++) {
		if (!ReleaseSemaphore(semaphoreItems, 1, NULL)) {
			Sleep(10);
			i--;
		}
	}

	for (int i = 0; i < nWorkers; i++) {
		WaitForSingleObject(threadHWorker[i], INFINITE);
	}

	WaitForSingleObject(threadHMonitor, INFINITE);

	for (int i = 0; i < nWorkers; i++) {
		CloseHandle(threadHWorker[i]);
	}

	CloseHandle(threadHMonitor);

	free(threadIDWorker);
	free(threadHWorker);

	if (logFile) fclose(logFile);

	CloseHandle(mutexList);
	CloseHandle(mutexOutput);
	CloseHandle(mutexVariables);
	CloseHandle(semaphoreSlots);
	CloseHandle(semaphoreItems);

	printf("\n\nTodos os trabalhos concluidos. Total processado: %d\n", countProcessed);

	return 0;
}