
#include <stdio.h>
#include <windows.h>

#define MAX_LINE 256
int numWorkers = 0;
int minWorkTime = 0;
int maxWorkTime = 0;

typedef struct Order {
	int id;
	char customerName[50];
	int quantidade;
	float precoTotal;
	struct Order* next;
} Order;

void read_data(FILE* file) {


	char line[256];

	fgets(line, sizeof(line), file


		line[strcspn(line, "\n")] = '\0'; // remove newline

		char* token = strtok(line, ";");
		if (!token) continue;
		users[user_count].id = atoi(token);

		token = strtok(NULL, ";");
		if (!token) continue;
		strncpy(users[user_count].name, token, sizeof(users[user_count].name));

		token = strtok(NULL, ";");
		if (!token) continue;
		users[user_count].value1 = atoi(token);

		token = strtok(NULL, ";");
		if (!token) continue;
		users[user_count].value2 = atof(token);

		user_count++;
		if (user_count >= MAX) break;

	printf("Loaded %d users from %s\n", user_count, filename);
}

void save_data() {


	// Guardar dados no txt "utilizadores"
	FILE* file_users = fopen("utilizadores.txt", "w");
	if (!file_users) { printf(" Erro: Abrir ficheiro utilizadores\t - ESCRITA\n"); }
	else {
		printf(" Sucesso: Abrir ficheiro utilizadores\t - ESCRITA\n");
		for (i = 0; i < MAX; i++) {
			if (users[i].id != 0) {
				fprintf(file_users, "%4i\n%s\n%s\n%s\n", users[i].id, users[i].name, users[i].email, users[i].password);
			}
		} fclose(file_users);
	}

	// Guardar dados no txt "conteudos"
	FILE* file_contents = fopen("conteudos.txt", "w");
	if (!file_contents) { printf(" Erro: Abrir ficheiro conteudos\t - ESCRITA\n"); }
	else {
		printf(" Sucesso: Abrir ficheiro conteudos\t - ESCRITA\n");
		for (i = 0; i < MAX; i++) {
			if (contents[i].id != 0) {
				fprintf(file_contents, "%4i %4i %4i %4i\n%s\n%s\n%s\n%s\n", contents[i].id, contents[i].day, contents[i].month,
					contents[i].year, contents[i].title, contents[i].type, contents[i].author, contents[i].genre);
			}
		} fclose(file_contents);
	}

	// Guardar dados no txt "playlists"
	FILE* file_playlists = fopen("playlists.txt", "w");
	if (!file_playlists) { printf(" Erro: Abrir ficheiro playlists\t - ESCRITA\n"); }
	else {
		printf(" Sucesso: Abrir ficheiro playlists\t - ESCRITA\n");
		for (i = 0; i < MAX; i++) {
			if (playlists[i].id != 0) {
				fprintf(file_playlists, "%4i %4i\n%s\n%s\n", playlists[i].id, playlists[i].id_owner, playlists[i].name, playlists[i].type);
				for (j = 0; j < LIM; j++) {
					fprintf(file_playlists, "%4i", playlists[i].content_id[j]);
				} fprintf(file_playlists, "\n");
			}
		} fclose(file_playlists);
	}

	printf("\n ********** ~o~ **********\n\n ");
}

DWORD WINAPI worker(LPVOID T)
{
	int* threadID;
	threadID = (int*)T;
	while (1) {
		WaitForSingleObject(mutex, INFINITE);
		if (items < capacity) {
			items++;
			printf("\n Stored Box: %d\t\t ThreadID: %d", items, threadID);
			ReleaseMutex(mutex);
			Sleep(1000);
		}
		else {
			printf("\n Max capacity.\t\t ThreadID: %d", threadID);
			ReleaseMutex(mutex);
			Sleep(1000);
		}

	}
	return 0;
}

DWORD WINAPI monitor(LPVOID T)

{
	int* threadID;
	threadID = (int*)T;
	while (1) {
		WaitForSingleObject(mutex, INFINITE);
		if (items > 0) {
			items--;
			printf("\n Removed Box: %d\t\t ThreadID: %d", items, threadID);
			ReleaseMutex(mutex);
			Sleep(2000);
		}
		else {
			printf("\n Empty.\t\t ThreadID: %d", threadID);
			ReleaseMutex(mutex);
			Sleep(2000);
		}
	}
	return 0;
}

void mainErrorHandeling(int argc, char* argv[]) {
	if (argc <= 4) {
		printf("Too few arguments\n");
		exit(1);
	}

	if (atoi(argv[2]) == 0 && argv[2][0] != '0') {
		printf("Invalid number of workers\n");
		exit(1);
	}
	else {
		numWorkers = atoi(argv[2]);
	}
	
	if (atoi(argv[3]) == 0 && argv[3][0] != '0') {
		printf("Invalid minimum worker duration\n");
		exit(1);
	}

	if (atoi(argv[4]) == 0 && argv[4][0] != '0') {
		printf("Invalid maximum worker duration\n");
		exit(1);
	}
}

int main(int argc, char* argv[])
{

	int i;

	mainErrorHandeling(argc, argv);

	DWORD threadIDWorker[NCONS], threadIDMonitor;
	HANDLE threadHWorker[NCONS], threadHMonitor;

	mutex = CreateMutexA(NULL, FALSE, NULL);
	if (mutex == NULL) return 0;

	threadHMonitor = CreateThread(NULL, 0, monitor, (LPVOID)i, 0, &threadIDMonitor);
	if (threadHMonitor == NULL) return 0;

	for (i = 0; i < NPROD; i++) {
		threadHWorker[i] = CreateThread(NULL, 0, worker, (LPVOID)i, 0, &threadIDWorker[i]);
		if (threadHWorker[i] == NULL) return 0;
	}

	for (i = 0; i < NCONS; i++) {
		WaitForSingleObject(threadHWorker[i], INFINITE);
	}

	for (i = 0; i < NPROD; i++) {
		WaitForSingleObject(threadHMonitor, INFINITE);
	}

	for (i = 0; i < NCONS; i++) {
		CloseHandle(threadHWorker[i]);
	}

	CloseHandle(threadHMonitor[i]);

	return 0;
}