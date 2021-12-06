#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <mpi.h>

#define TAMANHO_MSG 100
#define ROOT 0
#define TAG 0

// Operacoes
#define OP_NADA 0
// Envio de mensagem
#define OP_ENVIAR_MSG 1                 // > solicitação para enviar mensagem [para:aux]
#define OP_ENVIAR_MSG_CONCEDIDA 2       // < solicitação para envio mensagem CONCEDIDA (enviar ponto a ponto)
#define OP_RECEBER_MSG 3                // < receber msg do [processo:aux] (ponto a ponto)
// Edição de linha
#define OP_EDITAR_LINHA 4               // > solicitação editar [linha:aux]
#define OP_EDITAR_LINHA_CONCEDIDA 5     // < solicitação para editar linha CONCEDIDA
#define OP_EDITAR_LINHA_NEGADA 6        // < solicitação para editar linha NEGADA


struct Comunicacao{
    int op;
    int aux;
    char msg[TAMANHO_MSG];
};

char * getMsg() {
    int r = rand() % 8;

    switch (r) {
        case 1:
            return "Gostaria de dizer que te acho legal demais.";
        case 2: 
            return "Vamos sair para beber a noite?";
        case 3:
            return "O chefe é um mala.";
        case 4: 
            return "Obrigado por me auxiliar.";
        case 5:
            return "Vamos conversar?";
        case 6: 
            return "Vishhhh fiz uma cagada.";
        case 7:
            return "Sabia que o joão caiu no chão.";
        case 8: 
            return "UUUUUUUUUuaaaaaahhhhh, yeah!.";
        default:
            return "Eita nóis";

    }   
}

void enviarOpNada(int para) {
    struct Comunicacao _env;
    _env.op = OP_NADA;
    MPI_Send(&_env,sizeof(struct Comunicacao),MPI_CHAR, para ,TAG,MPI_COMM_WORLD);
}

// ---------------------------------------------------
// OPERAÇÕES ROOT
void solicitacaoEnvioMsg(int de, int para) {
    struct Comunicacao _env1;
    _env1.op = OP_RECEBER_MSG;
    _env1.aux = de;
    MPI_Send(&_env1,sizeof(struct Comunicacao),MPI_CHAR, para ,TAG,MPI_COMM_WORLD);

    struct Comunicacao _env2;
    _env2.op = OP_ENVIAR_MSG_CONCEDIDA;
    MPI_Send(&_env2,sizeof(struct Comunicacao),MPI_CHAR, de ,TAG,MPI_COMM_WORLD);
    
}
// ---------------------------------------------------

// ---------------------------------------------------
// OPERAÇÕES PROCESSOS
void enviarMsg(int processoAtual, int para, char * msg, MPI_Status status) {
    struct Comunicacao _env;
    _env.op = OP_ENVIAR_MSG;
    _env.aux = para;
    MPI_Send(&_env,sizeof(struct Comunicacao),MPI_CHAR,ROOT,TAG,MPI_COMM_WORLD);

    struct Comunicacao _recEnvioMsg;
    MPI_Recv(&_recEnvioMsg,sizeof(struct Comunicacao), MPI_CHAR, ROOT, TAG, MPI_COMM_WORLD, &status);  

    if (_recEnvioMsg.op == OP_ENVIAR_MSG_CONCEDIDA) {
        struct Comunicacao _envMsg;
        strncpy(_envMsg.msg, msg, TAMANHO_MSG-1);
        MPI_Send(&_envMsg,sizeof(struct Comunicacao),MPI_CHAR, _env.aux, TAG,MPI_COMM_WORLD);
    }
}

void receberMsg(int processoAtual, int de, MPI_Status status) {
    struct Comunicacao _recMsg;
    MPI_Recv(&_recMsg,sizeof(struct Comunicacao), MPI_CHAR, de, TAG, MPI_COMM_WORLD, &status);
    printf("[%d] Mensagem de %d: %s\n", processoAtual, de, _recMsg.msg);
}
// ---------------------------------------------------
// ---------------------------------------------------

int main(int argc, char **argv)
{
    int rank, size;

    srand(time(NULL));

    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int step = 0;
    
    for (;;) {
        printf("[%d] step %d\n", rank, step);
        if (rank != ROOT) {

            if (rank == 1 && step == 2) {
                
                enviarMsg(rank, 2, getMsg(), status);

            } else {
                enviarOpNada(ROOT);
            }
            

            struct Comunicacao _rec;
            MPI_Recv(&_rec,sizeof(struct Comunicacao), MPI_CHAR, ROOT, TAG, MPI_COMM_WORLD, &status);

            // PROCESSO DEVE RECEBER MENSAGEM DE OUTRO PROCESSO
            if (_rec.op == OP_RECEBER_MSG) {
                receberMsg(rank, _rec.aux, status);
            }
        }   

        if (rank == ROOT) {
            int i;

            for (i=1; i<size; i++) {
                struct Comunicacao _rec;
                MPI_Recv(&_rec,sizeof(struct Comunicacao), MPI_CHAR, i, TAG, MPI_COMM_WORLD, &status);

                if (_rec.op != OP_NADA) {

                    // PROCESSO DESEJA ENVIAR MSG PARA OUTRO PROCESSO
                    if (_rec.op == OP_ENVIAR_MSG) {
                        solicitacaoEnvioMsg(i, _rec.aux);
                    }
                }

                enviarOpNada(i);
            }
        }     
        
        sleep(2);

        step++;
    }


    MPI_Finalize();
}

