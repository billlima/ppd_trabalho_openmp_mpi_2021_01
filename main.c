#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <mpi.h>

#define SLEEP_SEC 1
#define FILEPATH "data/file.txt"
#define FILEPATHTEMP "data/file.tmp"
#define MAX_LINHAS 10
#define MAX_TAM_LINHA 100
#define MAX_TAM_MSG 100
#define ROOT 0
#define TAG0 0


// Operacoes
#define OP_NADA 0
// Envio de mensagem
#define OP_ENVIAR_MSG 1                 // > solicitação para enviar mensagem [para:aux]
#define OP_ENVIAR_MSG_CONCEDIDA 2       // < solicitação para envio mensagem CONCEDIDA (enviar ponto a ponto)
#define OP_ENVIAR_MSG_NEGADA 3          // < solicitação para envio mensagem NEGADA (enviar ponto a ponto)
// Edição de linha
#define OP_EDITAR_LINHA 4               // > solicitação editar [linha:aux]
#define OP_EDITAR_LINHA_CONCEDIDA 5     // < solicitação para editar linha CONCEDIDA
#define OP_EDITAR_LINHA_NEGADA 6        // < solicitação para editar linha NEGADA
#define OP_EDICAO_LINHA_CONCLUIDA 7     // > concluir alteração de linha

struct EdicaoLinha{
    int linha;
    int stepInicio;
};

struct Comunicacao{
    int op;
    int de;
    int aux;
    char msg[MAX_TAM_MSG];
};

void print3(int proc, int step, char * msg) {
    printf("[step %d][proc %d] %s\n", step, proc, msg);
}

void criarArquivo() {
    FILE *file;
    file = fopen(FILEPATH, "w");

    int x;
    for (x=1; x<=MAX_LINHAS; x++) {
        fputs("\n", file);
    }
    fclose(file);
}

void imprimirArquivo() {
    FILE *file;
    file = fopen(FILEPATH, "r");
    
    char msg[MAX_TAM_LINHA];

    printf("\n-- CONTEUDO DO ARQUIVO COMPARTILHADO\n");
    printf("-- inicio do conteudo\n");
    
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    
    int l=1;
    while ((read = getline(&line, &len, file)) != -1) {
        printf("%d: %s", l, line);
        l++;
    }
    
    fclose(file);
    printf("-- fim do conteudo\n\n");
}

char * getLinhaArquivo(int linha) {
    FILE *file;
    file = fopen(FILEPATH, "r");
    
    char msg[MAX_TAM_LINHA];

    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    
    int l=1;
    while ((read = getline(&line, &len, file)) != -1) {
        l++;
        if (l == linha) {
            return line;   
        }
    }
    
    return "";
}

void substituirLinha(int line, char * text) {
    FILE * fPtr;
    FILE * fTemp;   
    
    char buffer[MAX_TAM_LINHA];
    int count;

    fPtr  = fopen(FILEPATH, "r");
    fTemp = fopen(FILEPATHTEMP, "w"); 

    count = 0;
    while ((fgets(buffer, MAX_TAM_LINHA, fPtr)) != NULL)
    {
        count++;

        if (count == line) {
            fputs(text, fTemp);
            fputs("\n", fTemp);
        } else {
            fputs(buffer, fTemp);
        }
    }

    fclose(fPtr);
    fclose(fTemp);

    remove(FILEPATH);

    rename(FILEPATHTEMP, FILEPATH);

    printf("Linha %d atualizada\n", line);
}

char * getRandomMsg() {
    int r = (rand() % 8) + 1;

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

void limparRespostas(int *respostas, int size) {
    int x;
    for (x=0; x<size; x++) {
        respostas[x] = 0;
    }
}

void enviarOpNada(int para) {
    struct Comunicacao _env;
    _env.op = OP_NADA;
    MPI_Send(&_env,sizeof(struct Comunicacao),MPI_CHAR, para ,TAG0,MPI_COMM_WORLD);
}

struct Comunicacao toComunicacao(int *respostas, int x) {
    struct Comunicacao _c;
    _c.op = respostas[x];
    _c.de = respostas[x+1];
    _c.aux = respostas[x+2];
    return _c;
}

// ---------------------------------------------------
// OPERAÇÕES ROOT
void receberLinhaSubstituicao(int processoAtual, int step, int de, int linha, MPI_Status status) {
    struct Comunicacao _recMsg;
    MPI_Recv(&_recMsg,sizeof(struct Comunicacao), MPI_CHAR, de, TAG0, MPI_COMM_WORLD, &status);
    printf("Recebido alteracao do proc %d linha %d: %s\n", de, linha, _recMsg.msg);
    substituirLinha(linha, _recMsg.msg);
}
// ---------------------------------------------------

// ---------------------------------------------------
// OPERAÇÕES PROCESSOS
void enviarLinhaSubstituicao(int processoAtual, int step, char * msg) {
    struct Comunicacao _envMsg;
    strncpy(_envMsg.msg, msg, MAX_TAM_MSG-1);
    MPI_Send(&_envMsg,sizeof(struct Comunicacao),MPI_CHAR, ROOT, TAG0,MPI_COMM_WORLD);
}

void enviarMsg(int processoAtual, int step, int para, char * msg) {
    struct Comunicacao _envMsg;
    strncpy(_envMsg.msg, msg, MAX_TAM_MSG-1);
    MPI_Send(&_envMsg,sizeof(struct Comunicacao),MPI_CHAR, para, TAG0,MPI_COMM_WORLD);
}

void receberMsg(int processoAtual, int step, int de, MPI_Status status) {
    struct Comunicacao _recMsg;
    MPI_Recv(&_recMsg,sizeof(struct Comunicacao), MPI_CHAR, de, TAG0, MPI_COMM_WORLD, &status);
    printf("[step %d][proc %d] Mensagem de %d: %s\n", step, processoAtual, de, _recMsg.msg);
}

int getRandomLine() {
    return (rand() % MAX_LINHAS) + 1;
}

int getRandomProcess(int size, int dif) {
    int ret = dif;
    while(dif == ret) {
        ret = (rand() % size-1) + 1;
    }
    return ret;
}

struct Comunicacao getOperacaoProcesso(int processoAtual, int step, int size, struct EdicaoLinha *editandoLinha) {
    struct Comunicacao _com;
    _com.aux = 0;
    _com.op = OP_NADA;

    if (step % 4 == 0 && processoAtual == 1) {
        int para = getRandomProcess(size, processoAtual);
        _com.op = OP_ENVIAR_MSG;
        _com.aux = para;
        printf("[step %d][proc %d] Quero enviar mensagem para %d\n", step, processoAtual, _com.aux); 
    } 

    else if (step % 8 == 0 && processoAtual == 4) {
        int para = getRandomProcess(size, processoAtual);
        _com.op = OP_ENVIAR_MSG;
        _com.aux = para;
        printf("[step %d][proc %d] Quero enviar mensagem para %d\n", step, processoAtual, _com.aux); 
    } 

    else if (editandoLinha->linha == 0 && rand() % 10 == 1) {
        int linha = getRandomLine();
        _com.op = OP_EDITAR_LINHA;
        _com.aux = linha;
        printf("[step %d][proc %d] Quero editar a linha %d\n", step, processoAtual, _com.aux); 
    }

    else if (editandoLinha->linha != 0 && editandoLinha->stepInicio < (step - 8)) {
        if (rand() % 2 == 1) {
            _com.op = OP_EDICAO_LINHA_CONCLUIDA;
            _com.aux = editandoLinha->linha;

            editandoLinha->linha = 0;
            editandoLinha->stepInicio = 0;
        }
    }

    return _com;
}
// ---------------------------------------------------
// ---------------------------------------------------

int main(int argc, char **argv)
{
    int rank, size;

    int solicitacao[3];
    int * solicitacoes;
    int * respostas;

    srand(time(NULL));

    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int step = 0;
    int respostasSize = (size-1) * 3 * 2;
    int solicitacoesSize = size*3;
    // printf("size: %d, respostasSize: %d, solicitacoesSize %d\n", size, respostasSize, solicitacoesSize);

    if (rank == ROOT) {
        criarArquivo();
        imprimirArquivo();

        solicitacoes = (int *)malloc(solicitacoesSize * sizeof(int));
    }
    respostas = (int *)malloc(respostasSize * sizeof(int));

    struct EdicaoLinha *editandoLinha;
    editandoLinha = malloc(sizeof(struct EdicaoLinha));

    editandoLinha->linha = 0;
    editandoLinha->stepInicio = 0;

    int emEdicaoArr[MAX_LINHAS] = {0};

    MPI_Barrier(MPI_COMM_WORLD);
    
    for (;;) {

        solicitacao[0] = 0;
        solicitacao[1] = 0;
        solicitacao[2] = 0;

        if (step > 1) {
            // print3(rank, step, "");

            solicitacao[0] = OP_NADA;
            solicitacao[1] = rank;
            solicitacao[2] = 0;

            if (rank != ROOT) {
                
                struct Comunicacao _com = getOperacaoProcesso(rank, step, size, editandoLinha);
                solicitacao[0] = _com.op;
                solicitacao[2] = _com.aux;
                
            }

            MPI_Barrier(MPI_COMM_WORLD);

            MPI_Gather(solicitacao, 3, MPI_INT, solicitacoes, 3, MPI_INT, ROOT, MPI_COMM_WORLD);

            if (rank == ROOT) {
                int x;

                for (x=0; x<solicitacoesSize; x=x+3) {
                    struct Comunicacao _sol = toComunicacao(solicitacoes, x);

                    if (_sol.op != OP_NADA) {
                        int z = x;
                        
                        if (_sol.op == OP_ENVIAR_MSG) {
                            printf("[step %d][proc %d] Proc %d deseja enviar mensagem p/ %d, autorizado.\n", step, rank, _sol.de, _sol.aux);
                            respostas[z++] = OP_ENVIAR_MSG_CONCEDIDA;
                            respostas[z++] = _sol.de;
                            respostas[z++] = _sol.aux;
                        }

                        else if (_sol.op == OP_EDITAR_LINHA) {
                            int proc = emEdicaoArr[_sol.aux-1];

                            if (proc == 0 || proc == _sol.de) {
                                emEdicaoArr[_sol.aux-1] = _sol.de;

                                printf("[step %d][proc %d] Proc %d quer editar a linha %d, concedido.\n", step, rank, _sol.de, _sol.aux);
                                respostas[z++] = OP_EDITAR_LINHA_CONCEDIDA;
                                respostas[z++] = _sol.de;
                                respostas[z++] = _sol.aux;
                            } else {
                                printf("[step %d][proc %d] Proc %d quer editar a linha %d, NEGADO (já está em edição).\n", step, rank, _sol.de, _sol.aux);
                                respostas[z++] = OP_EDITAR_LINHA_NEGADA;
                                respostas[z++] = _sol.de;
                                respostas[z++] = proc;
                            }
                        }

                        else if (_sol.op == OP_EDICAO_LINHA_CONCLUIDA) {
                            emEdicaoArr[_sol.aux-1] = 0;

                            printf("[step %d][proc %d] Proc %d finalizou a alteração da linha %d, enviando aviso de alteracao\n", step, rank, _sol.de, _sol.aux);
                                respostas[z++] = OP_EDICAO_LINHA_CONCLUIDA;
                                respostas[z++] = _sol.de;
                                respostas[z++] = _sol.aux;
                        }

                    }
                }
            }

            MPI_Barrier(MPI_COMM_WORLD);

            MPI_Bcast(respostas, respostasSize, MPI_INT, ROOT, MPI_COMM_WORLD);
        
            int x;
            for(x=0; x<respostasSize; x=x+3) {
                struct Comunicacao _resp = toComunicacao(respostas, x);

                if (_resp.op != OP_NADA) {

                    if (rank != ROOT) {

                        if (_resp.de == rank) {
                            
                            switch (_resp.op) {
                                case OP_ENVIAR_MSG_CONCEDIDA:
                                    enviarMsg(rank, step, _resp.aux, getRandomMsg());
                                    break;
                                
                                case OP_EDITAR_LINHA_CONCEDIDA:
                                    printf("[step %d][proc %d] Edição linha %d iniciada\n", step, rank, _resp.aux);
                                    printf("[step %d][proc %d] Linha %d (atual): %s\n", step, rank, _resp.aux, getLinhaArquivo(_resp.aux));

                                    editandoLinha->linha = _resp.aux;
                                    editandoLinha->stepInicio = step;
                                    break;

                                case OP_EDITAR_LINHA_NEGADA:
                                    printf("[step %d][proc %d] Edição negada, já está sendo editada pelo proc %d\n", step, rank, _resp.aux);
                                    break;

                                case OP_EDICAO_LINHA_CONCLUIDA:
                                    enviarLinhaSubstituicao(rank, step, getRandomMsg());
                                    break;
                            }
                        }

                        else if (_resp.aux == rank && _resp.op == OP_ENVIAR_MSG_CONCEDIDA) {
                            receberMsg(rank, step, _resp.de, status);
                        }

                        else if (_resp.op == OP_EDICAO_LINHA_CONCLUIDA) {
                            // processo pode imprimirArquivo se tiver interesse.
                            // imprimirArquivo();
                        }

                    }

                    if (rank == ROOT && _resp.op == OP_EDICAO_LINHA_CONCLUIDA) {
                        receberLinhaSubstituicao(rank, step, _resp.de, _resp.aux, status);
                        imprimirArquivo();
                    } 

                }
            
            }
        } 

        MPI_Barrier(MPI_COMM_WORLD);

        if (rank == ROOT) {
            limparRespostas(respostas, respostasSize);
            printf(".\n");
        }
        
        usleep(1000 * 1000 * SLEEP_SEC);

        step++;
    }


    MPI_Finalize();
}

