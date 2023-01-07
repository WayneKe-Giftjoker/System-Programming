#include "status.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#define PLAYERNUM 8
#define BUFSIZE   1<<6

int main (int argc, char** argv) {
	int playerID = atoi(argv[1]);
	pid_t parentPID = atoi(argv[2]);
	FILE* allStat = fopen("player_status.txt", "r");
	Status myStat;
	char deli[1<<2] = " ";
	int init_HP;
	char target;
	if(0 <= playerID && playerID <= 7){ //real player
		//determine target
		switch(playerID){
			case 0: case 1:
				target = 'G';
				break;
			case 2: case 3:
				target = 'H';
				break;
			case 4: case 5:
				target = 'I';
				break;
			case 6: case 7:
				target = 'J';
				break;
		}
		//init
		int cnt = 0; char info[BUFSIZE];
		
		while(cnt < PLAYERNUM){
			fgets(info, BUFSIZE, allStat);
			if(cnt == playerID){
				break;
			}
			++cnt;
		}

		myStat.real_player_id = playerID;
		cnt = 0;
		char *tok = strtok(info, deli);
		while(tok != NULL){
			switch (cnt){
				case 0:
					myStat.HP = atoi(tok);
					init_HP = myStat.HP;
					break;
				case 1:
					myStat.ATK = atoi(tok);
					break;
				case 2:
					if(tok[0] == 'F')
						myStat.attr = FIRE;
					else if(tok[0] == 'W')
						myStat.attr = WATER;
					else
						myStat.attr = GRASS;
					break;
				case 3:
					myStat.current_battle_id = tok[0];
					break;
				case 4:
					myStat.battle_ended_flag = atoi(tok);
					break;
				
				default:
					break;
			}
			++cnt;
			tok = strtok(NULL, deli);
		}
		
		//start talking
		char buf[BUFSIZE];
		snprintf(buf, BUFSIZE, "log_player%d.txt", myStat.real_player_id);
		FILE *log = fopen(buf, "a");
		while(1){
			//write log before sending PSSM to target
			snprintf(buf, BUFSIZE, "%d,%d pipe to %c,%d %d,%d,%c,%d\n", playerID, getpid(), target, atoi(argv[2]), myStat.real_player_id, myStat.HP, myStat.current_battle_id, myStat.battle_ended_flag);
			fprintf(log, buf);
			write(STDOUT_FILENO, &myStat, sizeof(Status));
			//blocking read and write log after receive PSSM from target
			read(STDIN_FILENO, &myStat, sizeof(Status));
			snprintf(buf, BUFSIZE, "%d,%d pipe from %c,%d %d,%d,%c,%d\n", playerID, getpid(), target, atoi(argv[2]), myStat.real_player_id, myStat.HP, myStat.current_battle_id, myStat.battle_ended_flag);
			fprintf(log, buf);
			if(myStat.battle_ended_flag){
				myStat.battle_ended_flag = 0;
				if(myStat.HP <= 0){ //loser
					myStat.HP = init_HP;
					int agent;
					char *fifo_name;
					switch(myStat.current_battle_id){
						case 'B':
							fifo_name = "player14.fifo";
							agent = 14;
							break;
						case 'D':
							fifo_name = "player10.fifo";
							agent = 10;
							break;
						case 'E':
							fifo_name = "player13.fifo";
							agent = 13;
							break;
						case 'G':
							fifo_name = "player8.fifo";
							agent = 8;
							break;
						case 'H':
							fifo_name = "player11.fifo";
							agent = 11;
							break;
						case 'I':
							fifo_name = "player9.fifo";
							agent = 9;
							break;
						case 'J':
							fifo_name = "player12.fifo";
							agent = 12;
							break;
					}
					//write log before sending PSSM to agent
					if(myStat.current_battle_id != 'A'){
						snprintf(buf, BUFSIZE, "%d,%d fifo to %d %d,%d\n", playerID, getpid(), agent, myStat.real_player_id, myStat.HP);
						fprintf(log, buf);
						mkfifo(fifo_name, 0666);
						int fifo_w = open(fifo_name, O_WRONLY);
						write(fifo_w, &myStat, sizeof(Status));
						close(fifo_w);
					}
					fclose(allStat); fclose(log);
					exit(0);
				}
				else{ //winner
					myStat.HP += (init_HP - myStat.HP) / 2;
					if(myStat.current_battle_id == 'A'){
						fclose(allStat); fclose(log);
						break;
					}
				}
			}
		}
	}
	else{ //agent player
		//determine target, fifo and loser's id
		char *fifo_name;
		int loserID;
		switch(playerID){
			case 8:
				target = 'M';
				fifo_name = "player8.fifo";
				break;
			case 9:
				target = 'M';
				fifo_name = "player9.fifo";
				break;
			case 10:
				target = 'K';
				fifo_name = "player10.fifo";
				break;
			case 11:
				target = 'N';
				fifo_name = "player11.fifo";
				break;
			case 12:
				target = 'N';
				fifo_name = "player12.fifo";
				break;
			case 13:
				target = 'L';
				fifo_name = "player13.fifo";
				break;
			case 14:
				target = 'C';
				fifo_name = "player14.fifo";
				break;
		}

		//create fifo
		mkfifo(fifo_name, 0666);

		//read fifo data
		int fifo_r = open(fifo_name, O_RDONLY);
		read(fifo_r, &myStat, sizeof(Status));
		
		char buf[BUFSIZE];
		// if(playerID == 14)
		// 	fprintf(stderr, "hihi 14 right here!\n");
		snprintf(buf, BUFSIZE, "log_player%d.txt", myStat.real_player_id);
		FILE *log = fopen(buf, "a");
		//write log after receiving loser's PSSM
		snprintf(buf, BUFSIZE, "%d,%d fifo from %d %d,%d\n", playerID, getpid(), myStat.real_player_id, myStat.real_player_id, myStat.HP);
		fprintf(log, buf);

		//act like real player
		init_HP = myStat.HP;
		while(1){
			//write log before sending PSSM to target
			// if(playerID == 14)
			// 	fprintf(stderr, "hihi 14 send data!\n");
			snprintf(buf, BUFSIZE, "%d,%d pipe to %c,%d %d,%d,%c,%d\n", playerID, getpid(), target, atoi(argv[2]), myStat.real_player_id, myStat.HP, myStat.current_battle_id, myStat.battle_ended_flag);
			fprintf(log, buf);
			write(STDOUT_FILENO, &myStat, sizeof(Status));
			// if(playerID == 14)
			// 	fprintf(stderr, "hihi 14 read data!\n");
			//blocking read and write log after receive PSSM from target
			read(STDIN_FILENO, &myStat, sizeof(Status));
			snprintf(buf, BUFSIZE, "%d,%d pipe from %c,%d %d,%d,%c,%d\n", playerID, getpid(), target, atoi(argv[2]), myStat.real_player_id, myStat.HP, myStat.current_battle_id, myStat.battle_ended_flag);
			fprintf(log, buf);
			if(myStat.battle_ended_flag){
				myStat.battle_ended_flag = 0;
				if(myStat.HP <= 0){ //loser
					fclose(allStat); fclose(log); close(fifo_r);
					exit(0);
				}
				else{ //winner
					myStat.HP += (init_HP - myStat.HP) / 2;
					if(myStat.current_battle_id == 'A'){
						fclose(allStat); fclose(log); close(fifo_r);
						break;
					}
				}
			}
		}
	}
	return 0;
}