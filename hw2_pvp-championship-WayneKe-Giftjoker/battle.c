#include "status.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#define PLAYFLAG 0
#define PASSFLAG 1
#define DEADFLAG 2
#define L        0
#define R        1
#define BUFSIZE  1<<6

pid_t forkbattle(char targetID, int *pipe_r, int *pipe_w){
	int pipefd[2][2]; //[0] -> parent R child W || [1] -> child R parent W
	pipe(pipefd[0]); pipe(pipefd[1]);
	pid_t childpid;
	// char childBattleID[2]; childBattleID[0] = targetID; childBattleID[1] = '\n';
	// fprintf(stderr, "begin to fork child %c!\n", targetID);
	if((childpid = fork()) == 0){ //child
		close(pipefd[0][0]); close(pipefd[1][1]);
		dup2(pipefd[0][1], STDOUT_FILENO); dup2(pipefd[1][0], STDIN_FILENO);
		char arg1[BUFSIZE]; char arg2[BUFSIZE];
		snprintf(arg1, BUFSIZE, "%c", targetID); snprintf(arg2, BUFSIZE, "%d", getpid());
		execl("./battle", "battle", arg1, arg2, (char*) NULL);
	}
	close(pipefd[0][1]); close(pipefd[1][0]);
	*pipe_r = pipefd[0][0]; *pipe_w = pipefd[1][1];
	return childpid;
}

pid_t forkplayer(int targetID, int *pipe_r, int *pipe_w){
	int pipefd[2][2]; //[0] -> parent R child W || [1] -> child R parent W
	pipe(pipefd[0]); pipe(pipefd[1]);
	pid_t childpid;
	// char childBattleID[2]; childBattleID[0] = targetID; childBattleID[1] = '\n';
	if((childpid = fork()) == 0){ //child
		close(pipefd[0][0]); close(pipefd[1][1]);
		dup2(pipefd[0][1], STDOUT_FILENO); dup2(pipefd[1][0], STDIN_FILENO);
		char arg1[BUFSIZE]; char arg2[BUFSIZE];
		snprintf(arg1, BUFSIZE, "%d", targetID); snprintf(arg2, BUFSIZE, "%d", getpid());
		execl("./player", "player", arg1, arg2, (char*) NULL);
	}
	close(pipefd[0][1]); close(pipefd[1][0]);
	*pipe_r = pipefd[0][0]; *pipe_w = pipefd[1][1];
	return childpid;
}

void battle(Status* pssmL, Status* pssmR, Attribute stageAttr, char battleID){
	pssmL->current_battle_id = battleID; pssmR->current_battle_id = battleID;
	if(pssmL->HP < pssmR->HP){
		pssmR->HP -= ((pssmL->attr == stageAttr)? 2 : 1) * pssmL->ATK;
		if(pssmR->HP <= 0){
			pssmL->battle_ended_flag = 1; pssmR->battle_ended_flag = 1;
			return;
		}
		pssmL->HP -= ((pssmR->attr == stageAttr)? 2: 1) * pssmR->ATK;
		if(pssmL->HP <= 0){
			pssmL->battle_ended_flag = 1; pssmR->battle_ended_flag = 1;
			return;
		}
	}
	else if(pssmL->HP > pssmR->HP){
		pssmL->HP -= ((pssmR->attr == stageAttr)? 2 : 1) * pssmR->ATK;
		if(pssmL->HP <= 0){
			pssmL->battle_ended_flag = 1; pssmR->battle_ended_flag = 1;
			return;
		}
		pssmR->HP -= ((pssmL->attr == stageAttr)? 2 : 1) * pssmL->ATK;
		if(pssmR->HP <= 0){
			pssmL->battle_ended_flag = 1; pssmR->battle_ended_flag = 1;
			return;
		}
	}
	else{
		if(pssmL->real_player_id < pssmR->real_player_id){
			pssmR->HP -= ((pssmL->attr == stageAttr)? 2 : 1) * pssmL->ATK;
			if(pssmR->HP <= 0){
				pssmL->battle_ended_flag = 1; pssmR->battle_ended_flag = 1;
				return;
			}
			pssmL->HP -= ((pssmR->attr == stageAttr)? 2: 1) * pssmR->ATK;
			if(pssmL->HP <= 0){
				pssmL->battle_ended_flag = 1; pssmR->battle_ended_flag = 1;
				return;
			}
		}
		else if(pssmL->real_player_id > pssmR->real_player_id){
			pssmL->HP -= ((pssmR->attr == stageAttr)? 2 : 1) * pssmR->ATK;
			if(pssmL->HP <= 0){
				pssmL->battle_ended_flag = 1; pssmR->battle_ended_flag = 1;
				return;
			}
			pssmR->HP -= ((pssmL->attr == stageAttr)? 2: 1) * pssmL->ATK;
			if(pssmR->HP <= 0){
				pssmL->battle_ended_flag = 1; pssmR->battle_ended_flag = 1;
				return;
			}
		}
		else{
			fprintf(stderr, "Cannot fight oneself!\n");
		}
	}
}

void writeData(Status* pssm, int des, FILE* log, char buf[BUFSIZE], char selfID, int targetID, int targetPID, int leaf){
	if(!leaf)
		snprintf(buf, BUFSIZE, "%c,%d pipe to %c,%d %d,%d,%c,%d\n", selfID, getpid(), (char)targetID, targetPID, pssm->real_player_id, pssm->HP, pssm->current_battle_id, pssm->battle_ended_flag);
	else
		snprintf(buf, BUFSIZE, "%c,%d pipe to %d,%d %d,%d,%c,%d\n", selfID, getpid(), targetID, targetPID, pssm->real_player_id, pssm->HP, pssm->current_battle_id, pssm->battle_ended_flag);
	fprintf(log, buf);
	write(des, pssm, sizeof(Status));
}

void readData(Status* pssm, int src, FILE* log, char buf[BUFSIZE], char selfID, int targetID, int targetPID, int leaf){
	read(src, pssm, sizeof(Status));
	if(!leaf)
		snprintf(buf, BUFSIZE, "%c,%d pipe from %c,%d %d,%d,%c,%d\n", selfID, getpid(), (char)targetID, targetPID, pssm->real_player_id, pssm->HP, pssm->current_battle_id, pssm->battle_ended_flag);
	else
		snprintf(buf, BUFSIZE, "%c,%d pipe from %d,%d %d,%d,%c,%d\n", selfID, getpid(), targetID, targetPID, pssm->real_player_id, pssm->HP, pssm->current_battle_id, pssm->battle_ended_flag);
	fprintf(log, buf);
}

int main (int argc, char **argv) {
	char battleID = argv[1][0];
	char parentID;
	pid_t parentPID = atoi(argv[2]);
	int flag = PLAYFLAG;
	Attribute stageAttr;
	int targetL, targetR;
	int leafL, leafR;
	int winner; //1 -> left || 2 -> right
	char buf[BUFSIZE];
	FILE* log;
	
	int pipe_r[2], pipe_w[2]; //[0] -> for childL (left) || [1] -> for childR (right)
	pid_t childL, childR; //1 -> left || 2 -> right
	Status pssmL, pssmR;
	switch(battleID){
		case 'A':
			log = fopen("log_battleA.txt", "a");
			leafL = leafR = 0;
			stageAttr = FIRE;
			targetL = (int)'B'; targetR = (int)'C';
			childL = forkbattle((char)targetL, &pipe_r[L], &pipe_w[L]); childR = forkbattle((char)targetR, &pipe_r[R], &pipe_w[R]);
			break;
		case 'B':
			log = fopen("log_battleB.txt", "a");
			parentID = 'A';
			leafL = leafR = 0;
			stageAttr = GRASS;
			targetL = (int)'D'; targetR = (int)'E';
			childL = forkbattle((char)targetL, &pipe_r[L], &pipe_w[L]); childR = forkbattle((char)targetR, &pipe_r[R], &pipe_w[R]);
			break;
		case 'C':
			log = fopen("log_battleC.txt", "a");
			parentID = 'A';
			leafL = 0; leafR = 1;
			stageAttr = WATER;
			targetL = (int)'F'; targetR = 14;
			childL = forkbattle((char)targetL, &pipe_r[L], &pipe_w[L]); childR = forkplayer(targetR, &pipe_r[R], &pipe_w[R]);
			break;
		case 'D':
			log = fopen("log_battleD.txt", "a");
			parentID = 'B';
			leafL = leafR = 0;
			stageAttr = WATER;
			targetL = (int)'G'; targetR = (int)'H';
			childL = forkbattle((char)targetL, &pipe_r[L], &pipe_w[L]); childR = forkbattle((char)targetR, &pipe_r[R], &pipe_w[R]);
			break;
		case 'E':
			log = fopen("log_battleE.txt", "a");
			parentID = 'B';
			leafL = leafR = 0;
			stageAttr = FIRE;
			targetL = (int)'I'; targetR = (int)'J';
			childL = forkbattle((char)targetL, &pipe_r[L], &pipe_w[L]); childR = forkbattle((char)targetR, &pipe_r[R], &pipe_w[R]);
			break;
		case 'F':
			log = fopen("log_battleF.txt", "a");
			parentID = 'C';
			leafL = leafR = 0;
			stageAttr = FIRE;
			targetL = (int)'K'; targetR = (int)'L';
			childL = forkbattle((char)targetL, &pipe_r[L], &pipe_w[L]); childR = forkbattle((char)targetR, &pipe_r[R], &pipe_w[R]);
			break;
		case 'G':
			log = fopen("log_battleG.txt", "a");
			parentID = 'D';
			leafL = leafR = 1;
			stageAttr = FIRE;
			targetL = 0; targetR = 1;
			childL = forkplayer(targetL, &pipe_r[L], &pipe_w[L]); childR = forkplayer(targetR, &pipe_r[R], &pipe_w[R]);
			break;
		case 'H':
			log = fopen("log_battleH.txt", "a");
			parentID = 'D';
			leafL = leafR = 1;
			stageAttr = GRASS;
			targetL = 2; targetR = 3;
			childL = forkplayer(targetL, &pipe_r[L], &pipe_w[L]); childR = forkplayer(targetR, &pipe_r[R], &pipe_w[R]);
			break;
		case 'I':
			log = fopen("log_battleI.txt", "a");
			parentID = 'E';
			leafL = leafR = 1;
			stageAttr = WATER;
			targetL = 4; targetR = 5;
			childL = forkplayer(targetL, &pipe_r[L], &pipe_w[L]); childR = forkplayer(targetR, &pipe_r[R], &pipe_w[R]);
			break;
		case 'J':
			log = fopen("log_battleJ.txt", "a");
			parentID = 'E';
			leafL = leafR = 1;
			stageAttr = GRASS;
			targetL = 6; targetR = 7;
			childL = forkplayer(targetL, &pipe_r[L], &pipe_w[L]); childR = forkplayer(targetR, &pipe_r[R], &pipe_w[R]);
			break;
		case 'K':
			log = fopen("log_battleK.txt", "a");
			parentID = 'F';
			leafL = 0; leafR = 1;
			stageAttr = GRASS;
			targetL = (int)'M'; targetR = 10;
			childL = forkbattle((char)targetL, &pipe_r[L], &pipe_w[L]); childR = forkplayer(targetR, &pipe_r[R], &pipe_w[R]);
			break;
		case 'L':
			log = fopen("log_battleL.txt", "a");
			parentID = 'F';
			leafL = 0; leafR = 1;
			stageAttr = GRASS;
			targetL = (int)'N'; targetR = 13;
			childL = forkbattle((char)targetL, &pipe_r[L], &pipe_w[L]); childR = forkplayer(targetR, &pipe_r[R], &pipe_w[R]);
			break;
		case 'M':
			log = fopen("log_battleM.txt", "a");
			parentID = 'K';
			leafL = leafR = 1;
			stageAttr = FIRE;
			targetL = 8; targetR = 9;
			childL = forkplayer(targetL, &pipe_r[L], &pipe_w[L]); childR = forkplayer(targetR, &pipe_r[R], &pipe_w[R]);
			break;
		case 'N':
			log = fopen("log_battleN.txt", "a");
			parentID = 'L';
			leafL = leafR = 1;
			stageAttr = WATER;
			targetL = 11; targetR = 12;
			childL = forkplayer(targetL, &pipe_r[L], &pipe_w[L]); childR = forkplayer(targetR, &pipe_r[R], &pipe_w[R]);
			break;
	}

	while(flag == PLAYFLAG){
		readData(&pssmL, pipe_r[L], log, buf, battleID, targetL, childL, leafL); readData(&pssmR, pipe_r[R], log, buf, battleID, targetR, childR, leafR);
		battle(&pssmL, &pssmR, stageAttr, battleID);
		if(pssmL.battle_ended_flag == 1){
			flag = PASSFLAG;
			if(pssmL.HP > 0) winner = L;
			else winner = R;
		}
		writeData(&pssmL, pipe_w[L], log, buf, battleID, targetL, childL, leafL); writeData(&pssmR, pipe_w[R], log, buf, battleID, targetR, childR, leafR);
	}

	pid_t status;
	if(battleID != 'A'){
		Status pssmW;
		int pipeW_r, pipeW_w;
		int targetW;
		pid_t childW;
		int leafW;
		if(winner == L){
			pipeW_r = pipe_r[L]; pipeW_w = pipe_w[L];
			targetW = targetL;
			childW = childL;
			leafW = leafL;
			waitpid(childR, &status, WNOHANG);
		}
		else{
			pipeW_r = pipe_r[R]; pipeW_w = pipe_w[R];
			targetW = targetR;
			childW = childR;
			leafW = leafR;
			waitpid(childL, &status, WNOHANG);
		}
		while(flag == PASSFLAG){
			//read from child
			readData(&pssmW, pipeW_r, log, buf, battleID, targetW, childW, leafW);
			//write to parent
			writeData(&pssmW, STDOUT_FILENO, log, buf, battleID, (int)parentID, parentPID, 0);
			//read from parent
			readData(&pssmW, STDIN_FILENO, log, buf, battleID, (int)parentID, parentPID, 0);
			if(pssmW.HP <= 0 || (pssmW.current_battle_id == 'A' && pssmW.battle_ended_flag == 1)){
				flag = DEADFLAG;
			}
			//write to child
			writeData(&pssmW, pipeW_w, log, buf, battleID, targetW, childW, leafW);
		}

		int cnt = 1;
		while(cnt){
			if(waitpid(childW, &status, 0) == childW)
				--cnt;
		}
	}
	else if(battleID == 'A'){
		if(winner == L) fprintf(stdout, "Champion is P%d\n", pssmL.real_player_id);
		else fprintf(stdout, "Champion is P%d\n", pssmR.real_player_id);
		int cnt = 2;
		while(cnt){
			if(waitpid(childL, &status, 0) == childL)
				--cnt;
			childL = childR;
		}
	}
	// fprintf(stderr, "%c suicide!\n", battleID);
    return 0;
}