#include <stdio.h>
#include <string.h>
#include "utils.h"
#include "assemble.h"

int main(int argc, char **argv) {
    char hexTable[16]={'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
    char *instructions[] = {"add","sub","slt","or","nand",
    "addi","slti","ori","lui","lw","sw","beq","jalr",
    "j","halt"};
    char * directives[] = {".fill", ".space"};

    FILE *assp,*machp;
    struct symbolTable *pSymTab;
    size_t symTabLen;
    int found,noInsts;
    if(argc < 3){
      printf("***** Please run this program as follows:\n");
      printf("***** %s assprog.as machprog.m\n",argv[0]);
      printf("***** where assprog.as is your assembly program\n");
      printf("***** and machprog.m will be your machine code.\n");
      exit(EXIT_FAILIURE);
    }
    if((assp=fopen(argv[1],"r")) == NULL){
      printf("%s cannot be opened\n",argv[1]);
      exit(EXIT_FAILIURE);
    }
    if((machp=fopen(argv[2],"w+")) == NULL){
      printf("%s cannot be opened\n",argv[2]);
      exit(EXIT_FAILIURE);
    }
    // create and fill symbol table (first scan)
    symTabLen = findSymTabLen(assp);
    pSymTab = (struct symbolTable *) malloc(symTabLen * sizeof(struct symbolTable));
    fillSymTab(pSymTab, assp);
    for (int x = 0 ; x<symTabLen; x++) {
        printf("%s\t%d\n", pSymTab[x].symbol, pSymTab[x].value);
    }

    // find and form instructions (second scan)

    size_t lineSize;
    int PC = 0;
    int instCnt=0;
    char *line;
    char *token;
    int lineNo = 0;
    char instructParts[6][10];
    struct instruction *currInst;
    currInst = (struct instruction *) malloc(sizeof(struct instruction));
    line = (char *)malloc(72);
    while(getline(&line, &lineSize, assp) != -1) {
        lineNo++;
        if(line[0] == '\n' || line[0] == '#') continue;
        printf("%s", line);
        int i = 0, found = 0;
        token = strtok(line ,"\t, \n");
        while (token != NULL) {
            if (token[0] == '#') break;
            strcpy(instructParts[i++], token);
            token = strtok(NULL, "\t, \n");
        }

        int pos, instNo = 15, dirNo = 2;
        // check wether or not it is a directive
        for (pos=0; pos<2 && found!=1; pos++) {
            for (dirNo=0; dirNo<2; dirNo++) {
                if (strcmp(instructParts[pos], directives[dirNo]) == 0) {
                    found = 1;
                    pos--;
                    break;
                }
            }
        }
        // check wether first or second word is an instruction
        if (found!=1)
        for (pos=0; pos<2 && found!=1; pos++) {
            for (instNo=0; instNo<15; instNo++) {
                if (strcmp(instructParts[pos], instructions[instNo]) == 0) {
                    found = 1;
                    pos--;
                    break;
                }
            }
        }
        //printf("%d, %d\n", dirNo, instNo);

        currInst->PC = PC;
        if (dirNo < 2) {
            int immidiate = atoiImproved(instructParts[++pos], pSymTab, symTabLen);
            switch(dirNo) {
            case 0:
                fprintf(machp, "%d\n", immidiate);

            break;

            case 1:
            // allocates space in memory. not for this phase of project

            break;
            default:
                printf("invalid dirNo!\n");
                exit(EXIT_FAILIURE);

            }
            PC++;
            instCnt++;
        }
        else if (instNo < 15) {
            currInst->intInst = instNo;

            strcpy(currInst->inst, instructions[instNo]);

            if (isRType(instNo)) {
                currInst->instType = RTYPE;
                currInst->rd = atoiImproved(instructParts[++pos], pSymTab, -1);
                currInst->rs = atoiImproved(instructParts[++pos], pSymTab, -1);
                currInst->rt = atoiImproved(instructParts[++pos], pSymTab, -1);
                sprintf(currInst->mnemonic, "%s\t%d,%d,%d",
                        instructions[currInst->intInst],
                        currInst->rd,
                        currInst->rs,
                        currInst->rt);
                formInst(currInst);
                fprintf(machp, "%d\n", hexToInt(currInst->hexInst));

            }
            else if (isIType(instNo)) {
                currInst->instType = ITYPE;
                currInst->rt = atoiImproved(instructParts[++pos], pSymTab, -1);
                if (instructions[instNo] == "jalr") {
                    currInst->rs = atoiImproved(instructParts[++pos], pSymTab, -1);
                    sprintf(currInst->mnemonic, "%s\t%d,%d",
                            instructions[currInst->intInst],
                            currInst->rt,
                            currInst->rs);

                }
                else if (instructions[instNo] == "lui") {
                    currInst->imm = atoiImproved(instructParts[++pos], pSymTab, symTabLen);
                    sprintf(currInst->mnemonic, "%s\t%d,%d",
                            instructions[currInst->intInst],
                            currInst->rt,
                            currInst->imm);

                }
                else {
                    currInst->rs = atoiImproved(instructParts[++pos], pSymTab, -1);
                    currInst->imm = atoiImproved(instructParts[++pos], pSymTab, symTabLen);
                    if (strcmp(instructions[instNo], "beq") == 0) currInst->imm -= (PC+1);// relative addressing for beq
                    if (currInst->imm > 1<<16) {
                        printf("offset bigger than 2^16 in line %d.\n", lineNo);
                        exit(EXIT_FAILIURE);
                    }

                    sprintf(currInst->mnemonic, "%s\t%d,%d,%d",
                            instructions[currInst->intInst],
                            currInst->rt,
                            currInst->rs,
                            currInst->imm);
                }
                formInst(currInst);
                fprintf(machp, "%d\n", hexToInt(currInst->hexInst));

            }
            else if (isJType(instNo)) {
                currInst->instType = JTYPE;
                if (strcmp(instructions[instNo], "j") == 0) {
                    currInst->imm = atoiImproved(instructParts[++pos], pSymTab, symTabLen);
                    sprintf(currInst->mnemonic, "j\t%d", currInst->imm);
                }
                else if (strcmp(instructions[instNo], "halt") == 0) {
                    currInst->imm = 0;
                    strcpy(currInst->mnemonic, "halt");
                }

                formInst(currInst);
                fprintf(machp, "%d\n", hexToInt(currInst->hexInst));
            }
            PC++;
            instCnt++;
        }
        else {
        // instruction does not exist
            printf("invalid instruction in line %d.\n", lineNo);
            exit(EXIT_FAILIURE);

        }

        printf("hex instruction: %s\n", currInst->hexInst);
        printf("PC value: %d\n", currInst->PC);

        printf("----------------------------------------\n");
    }



    free(line);
    free(currInst);


    free(pSymTab);


    fclose(assp);
    fclose(machp);
    return 0;
}

