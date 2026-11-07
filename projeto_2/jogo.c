#include "raylib.h"
#include "stdio.h" 
#include "stdlib.h"
#include "math.h"
#include "string.h"

#define GRID_LINHAS 10
#define GRID_COLUNAS 10

typedef enum {
    ESTADO_MENU,
    ESTADO_VERTICAL,
    ESTADO_HORIZONTAL,
    ESTADO_GAMEOVER,
    ESTADO_RANKING
} EstadoJogo;

typedef enum {
    DIR_ESQUERDA,
    DIR_DIREITA
} Direcao;

typedef enum {
    SAPO_BASE,
    SAPO_PULANDO,
    SAPO_NO_AR
} EstadoSapo;

typedef struct {
    float x, y;
} Ponto2D;

typedef struct {
    Ponto2D pos;
    Vector2 vel;
    bool ativa;
    Color cor;
    int frameAtual;
    float tempoFrame;
} Mosca;

typedef struct {
    Ponto2D pos;
    Vector2 vel;
    bool noChao;
    Direcao direcao;
    int pontos;
    float tempoAnimacao;
    EstadoSapo animState;
} Sapo;

typedef struct {
    Rectangle rect;
    Color cor;
} Plataforma;

typedef struct {
    Sapo sapo;
    
    Mosca *moscas;            
    int numMoscas;
    
    Plataforma *plataformas;   
    int numPlataformas;

    EstadoJogo estado;
    char msgStatus[100];     

    Texture2D texSapoBase;
    Texture2D texSapoPuloInicial;
    Texture2D texSapoNoAr;
    Texture2D texSapoMorto;
    
    Texture2D texMosca[5];

    float camY;          
    float camX;
    float altMax;   
    float yUltimaPlat;  
    float xUltimaPlat;

    float tempoGameover; 

    int bgGrid[GRID_LINHAS][GRID_COLUNAS];

    int scores[10];
    int numScores;
} Jogo;

Jogo* CriarJogo(int largura, int altura);
void ReiniciarJogo(Jogo *jogo, EstadoJogo novoEstado);
void AtualizarJogo(Jogo *jogo);
void DesenharJogo(const Jogo *jogo);
void DestruirJogo(Jogo *jogo);
void GerarMundoVertical(Jogo *jogo);
void GerarMundoHorizontal(Jogo *jogo);
void SalvarPontuacao(Jogo *jogo);
void CarregarPontuacoes(Jogo *jogo);
void OrdenarScores(Jogo *jogo);
int BuscarScore(Jogo *jogo, int valor);

int main(void) {
    const int larguraTela = 800;
    const int alturaTela = 600;

    InitWindow(larguraTela, alturaTela, "Jogo do Sapo - Subida Infinita");
    SetTargetFPS(60);

    Jogo *jogo = CriarJogo(larguraTela, alturaTela);

    while (!WindowShouldClose()) {
        AtualizarJogo(jogo);

        BeginDrawing();
        ClearBackground(RAYWHITE);
        DesenharJogo(jogo);
        EndDrawing();
    }

    DestruirJogo(jogo);
    CloseWindow();

    return 0;
}

Jogo* CriarJogo(int largura, int altura) {
    Jogo *jogo = (Jogo*) malloc(sizeof(Jogo));

    jogo->texSapoBase = LoadTexture("assets/sprites/sapo_base.png");
    jogo->texSapoPuloInicial = LoadTexture("assets/sprites/sapo_pulo_inicial.png");
    jogo->texSapoNoAr = LoadTexture("assets/sprites/sapo_no_ar.png");
    jogo->texSapoMorto = LoadTexture("assets/sprites/sapo_morto.png");
    
    jogo->texMosca[0] = LoadTexture("assets/sprites/mosca_normal.png");
    jogo->texMosca[1] = LoadTexture("assets/sprites/mosca_bater_asas1.png");
    jogo->texMosca[2] = LoadTexture("assets/sprites/mosca_bater_asas2.png");
    jogo->texMosca[3] = LoadTexture("assets/sprites/mosca_bater_asas3.png");
    jogo->texMosca[4] = LoadTexture("assets/sprites/mosca_bater_asas4.png");
    
    jogo->moscas = NULL;
    jogo->plataformas = NULL;

    for (int l = 0; l < GRID_LINHAS; l++) {
        for (int c = 0; c < GRID_COLUNAS; c++) {
            jogo->bgGrid[l][c] = (l + c) % 2;
        }
    }

    jogo->estado = ESTADO_MENU;
    jogo->numScores = 0;
    CarregarPontuacoes(jogo);

    return jogo;
}

void ReiniciarJogo(Jogo *jogo, EstadoJogo novoEstado) {
    jogo->estado = novoEstado;

    jogo->sapo.pos.x = 100.0f;
    jogo->sapo.pos.y = 400.0f;
    jogo->sapo.vel.x = 0;
    jogo->sapo.vel.y = 0;
    jogo->sapo.noChao = false;
    jogo->sapo.direcao = DIR_DIREITA;
    jogo->sapo.pontos = 0;
    jogo->sapo.tempoAnimacao = 0.0f;
    jogo->sapo.animState = SAPO_BASE;
    jogo->tempoGameover = 0.0f;

    jogo->camY = 0.0f;
    jogo->camX = 0.0f;
    jogo->altMax = jogo->sapo.pos.y;

    snprintf(jogo->msgStatus, sizeof(jogo->msgStatus), "Moscas Capturadas: 0");

    if (jogo->plataformas != NULL) free(jogo->plataformas);
    if (jogo->moscas != NULL) free(jogo->moscas);

    if (novoEstado == ESTADO_VERTICAL) {
        jogo->numPlataformas = 5;
        jogo->plataformas = (Plataforma*) malloc(jogo->numPlataformas * sizeof(Plataforma));
        
        jogo->plataformas[0] = (Plataforma){ { 0, 550, 800, 50 }, DARKGREEN };
        jogo->plataformas[1] = (Plataforma){ { 300, 420, 200, 20 }, DARKBROWN };
        jogo->plataformas[2] = (Plataforma){ { 100, 300, 180, 20 }, DARKBROWN };
        jogo->plataformas[3] = (Plataforma){ { 500, 180, 180, 20 }, DARKBROWN };
        jogo->plataformas[4] = (Plataforma){ { 250, 60, 180, 20 }, DARKBROWN };

        jogo->yUltimaPlat = 60.0f;

        jogo->numMoscas = 2;
        jogo->moscas = (Mosca*) malloc(jogo->numMoscas * sizeof(Mosca));

        jogo->moscas[0] = (Mosca){ { 150, 220 }, { 2.0f, 0 }, true, DARKGRAY, GetRandomValue(0, 4), (float)GetRandomValue(0, 8) * 0.01f };
        jogo->moscas[1] = (Mosca){ { 550, 100 }, { -1.5f, 0 }, true, DARKGRAY, GetRandomValue(0, 4), (float)GetRandomValue(0, 8) * 0.01f };
    } else if (novoEstado == ESTADO_HORIZONTAL) {
        jogo->numPlataformas = 5;
        jogo->plataformas = (Plataforma*) malloc(jogo->numPlataformas * sizeof(Plataforma));
        
        jogo->plataformas[0] = (Plataforma){ { 0, 550, 800, 50 }, DARKGREEN };
        jogo->plataformas[1] = (Plataforma){ { 300, 420, 200, 20 }, DARKBROWN };
        jogo->plataformas[2] = (Plataforma){ { 600, 350, 200, 20 }, DARKBROWN };
        jogo->plataformas[3] = (Plataforma){ { 900, 280, 200, 20 }, DARKBROWN };
        jogo->plataformas[4] = (Plataforma){ { 1200, 200, 200, 20 }, DARKBROWN };

        jogo->xUltimaPlat = 1200.0f;

        jogo->numMoscas = 3;
        jogo->moscas = (Mosca*) malloc(jogo->numMoscas * sizeof(Mosca));

        jogo->moscas[0] = (Mosca){ { 350, 380 }, { 0, 0 }, true, DARKGRAY, GetRandomValue(0, 4), (float)GetRandomValue(0, 8) * 0.01f };
        jogo->moscas[1] = (Mosca){ { 650, 310 }, { 0, 0 }, true, DARKGRAY, GetRandomValue(0, 4), (float)GetRandomValue(0, 8) * 0.01f };
        jogo->moscas[2] = (Mosca){ { 950, 240 }, { 0, 0 }, true, DARKGRAY, GetRandomValue(0, 4), (float)GetRandomValue(0, 8) * 0.01f };
    }
}

void SalvarPontuacao(Jogo *jogo) {
    FILE *f = fopen("pontuacoes.dat", "ab");
    if (f != NULL) {
        fwrite(&jogo->sapo.pontos, sizeof(int), 1, f);
        fclose(f);
    }
    CarregarPontuacoes(jogo);
}

void CarregarPontuacoes(Jogo *jogo) {
    FILE *f = fopen("pontuacoes.dat", "rb");
    jogo->numScores = 0;
    if (f != NULL) {
        int val;
        while (fread(&val, sizeof(int), 1, f) == 1 && jogo->numScores < 10) {
            jogo->scores[jogo->numScores++] = val;
        }
        fclose(f);
    }
}

void OrdenarScores(Jogo *jogo) {
    for (int i = 0; i < jogo->numScores - 1; i++) {
        for (int j = 0; j < jogo->numScores - i - 1; j++) {
            if (jogo->scores[j] < jogo->scores[j+1]) {
                int temp = jogo->scores[j];
                jogo->scores[j] = jogo->scores[j+1];
                jogo->scores[j+1] = temp;
            }
        }
    }
}

int BuscarScore(Jogo *jogo, int valor) {
    OrdenarScores(jogo);
    int inicio = 0;
    int fim = jogo->numScores - 1;
    while (inicio <= fim) {
        int meio = inicio + (fim - inicio) / 2;
        if (jogo->scores[meio] == valor) return meio;
        if (jogo->scores[meio] < valor) fim = meio - 1;
        else inicio = meio + 1;
    }
    return -1;
}

void GerarMundoVertical(Jogo *jogo) {
    while (jogo->yUltimaPlat > jogo->altMax - 800.0f) {
        float dy = (float)GetRandomValue(130, 180);
        float novoY = jogo->yUltimaPlat - dy;
        float novoX = (float)GetRandomValue(50, 570);
        float largPlat = (float)GetRandomValue(160, 240);

        jogo->numPlataformas++;
        jogo->plataformas = (Plataforma*) realloc(jogo->plataformas, jogo->numPlataformas * sizeof(Plataforma));
        jogo->plataformas[jogo->numPlataformas - 1] = (Plataforma){ { novoX, novoY, largPlat, 20 }, DARKBROWN };

        jogo->yUltimaPlat = novoY;

        if (GetRandomValue(1, 100) <= 60) {
            jogo->numMoscas++;
            jogo->moscas = (Mosca*) realloc(jogo->moscas, jogo->numMoscas * sizeof(Mosca));

            float velX = (float)GetRandomValue(1, 3);
            if (GetRandomValue(0, 1) == 0) velX *= -1;

            jogo->moscas[jogo->numMoscas - 1] = (Mosca){ 
                { novoX + largPlat / 2.0f - 15.0f, novoY - 40.0f }, 
                { velX, 0 }, 
                true, 
                DARKGRAY,
                GetRandomValue(0, 4),
                (float)GetRandomValue(0, 8) * 0.01f
            };
        }
    }
}

void GerarMundoHorizontal(Jogo *jogo) {
    while (jogo->xUltimaPlat < jogo->sapo.pos.x + 1000.0f) {
        float dx = (float)GetRandomValue(200, 300);
        float novoX = jogo->xUltimaPlat + dx;
        float novoY = (float)GetRandomValue(200, 450);
        float largPlat = (float)GetRandomValue(160, 240);

        jogo->numPlataformas++;
        jogo->plataformas = (Plataforma*) realloc(jogo->plataformas, jogo->numPlataformas * sizeof(Plataforma));
        jogo->plataformas[jogo->numPlataformas - 1] = (Plataforma){ { novoX, novoY, largPlat, 20 }, DARKBROWN };

        jogo->xUltimaPlat = novoX;

        if (GetRandomValue(1, 100) <= 60) {
            jogo->numMoscas++;
            jogo->moscas = (Mosca*) realloc(jogo->moscas, jogo->numMoscas * sizeof(Mosca));

            jogo->moscas[jogo->numMoscas - 1] = (Mosca){ 
                { novoX + largPlat / 2.0f - 15.0f, novoY - 40.0f }, 
                { 0, 0 }, 
                true, 
                DARKGRAY,
                GetRandomValue(0, 4),
                (float)GetRandomValue(0, 8) * 0.01f
            };
        }
    }
}

void AtualizarJogo(Jogo *jogo) {
    if (jogo->estado == ESTADO_MENU) {
        if (IsKeyPressed(KEY_ONE)) ReiniciarJogo(jogo, ESTADO_VERTICAL);
        if (IsKeyPressed(KEY_TWO)) ReiniciarJogo(jogo, ESTADO_HORIZONTAL);
        if (IsKeyPressed(KEY_THREE)) {
            OrdenarScores(jogo);
            jogo->estado = ESTADO_RANKING;
        }
        return;
    }

    if (jogo->estado == ESTADO_RANKING) {
        if (IsKeyPressed(KEY_ENTER)) {
            jogo->estado = ESTADO_MENU;
        }
        return;
    }

    if (jogo->estado == ESTADO_GAMEOVER) {
        jogo->tempoGameover += GetFrameTime();
        if (jogo->tempoGameover >= 3.0f) {
            if (IsKeyPressed(KEY_R)) ReiniciarJogo(jogo, ESTADO_VERTICAL);
            if (IsKeyPressed(KEY_M)) jogo->estado = ESTADO_MENU;
        }
        return;
    }

    jogo->sapo.tempoAnimacao += GetFrameTime();

    const float velMovimento = 6.0f;
    const float gravidade = 0.6f;
    const float forcaPulo = -20.0f;

    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {
        jogo->sapo.pos.x -= velMovimento;
        jogo->sapo.direcao = DIR_ESQUERDA;
    }
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) {
        jogo->sapo.pos.x += velMovimento;
        jogo->sapo.direcao = DIR_DIREITA;
    }

    if (jogo->estado == ESTADO_VERTICAL) {
        if (jogo->sapo.pos.x < -25) jogo->sapo.pos.x = 800;
        if (jogo->sapo.pos.x > 800) jogo->sapo.pos.x = -25;
    } else {
        if (jogo->sapo.pos.x < 0) jogo->sapo.pos.x = 0;
    }

    jogo->sapo.pos.y += jogo->sapo.vel.y;
    jogo->sapo.vel.y += gravidade;

    Rectangle rectSapo = { jogo->sapo.pos.x, jogo->sapo.pos.y, 50, 50 };
    jogo->sapo.noChao = false;

    for (int i = 0; i < jogo->numPlataformas; i++) {
        if (jogo->sapo.vel.y >= 0 && CheckCollisionRecs(rectSapo, jogo->plataformas[i].rect)) {
            float peAnterior = (jogo->sapo.pos.y + 50) - jogo->sapo.vel.y;
            if (peAnterior <= jogo->plataformas[i].rect.y + 20.0f) {
                jogo->sapo.pos.y = jogo->plataformas[i].rect.y - 50;
                jogo->sapo.vel.y = 0;
                jogo->sapo.noChao = true;
                break;
            }
        }
    }

    bool pular = IsKeyDown(KEY_SPACE) || IsKeyDown(KEY_UP) || IsKeyDown(KEY_W);
    if (pular && jogo->sapo.noChao) {
        jogo->sapo.vel.y = forcaPulo;
        jogo->sapo.noChao = false;
        jogo->sapo.animState = SAPO_PULANDO;
    }

    if (jogo->sapo.noChao) {
        jogo->sapo.animState = SAPO_BASE;
    } else {
        if (jogo->sapo.animState == SAPO_PULANDO && jogo->sapo.vel.y >= 0) {
            jogo->sapo.animState = SAPO_NO_AR;
        }
    }

    if (jogo->estado == ESTADO_VERTICAL) {
        if (jogo->sapo.pos.y < jogo->altMax) {
            jogo->altMax = jogo->sapo.pos.y;
        }
        
        float alvoCamY = jogo->altMax - 300.0f;
        if (alvoCamY < jogo->camY) {
            jogo->camY = alvoCamY;
        }

        GerarMundoVertical(jogo);

        if (jogo->sapo.pos.y > jogo->camY + 650.0f) {
            SalvarPontuacao(jogo);
            jogo->tempoGameover = 0.0f;
            jogo->estado = ESTADO_GAMEOVER;
            return;
        }
    } else {
        float alvoCamX = jogo->sapo.pos.x - 300.0f;
        if (alvoCamX > jogo->camX) {
            jogo->camX = alvoCamX;
        }

        GerarMundoHorizontal(jogo);

        if (jogo->sapo.pos.y > 650.0f) {
            SalvarPontuacao(jogo);
            jogo->tempoGameover = 0.0f;
            jogo->estado = ESTADO_GAMEOVER;
            return;
        }
    }

    for (int i = 0; i < jogo->numMoscas; i++) {
        if (!jogo->moscas[i].ativa) continue;

        jogo->moscas[i].tempoFrame += GetFrameTime();
        if (jogo->moscas[i].tempoFrame >= 0.1f) {
            jogo->moscas[i].frameAtual = (jogo->moscas[i].frameAtual + 1) % 5;
            jogo->moscas[i].tempoFrame = 0.0f;
        }

        if (jogo->estado == ESTADO_VERTICAL) {
            jogo->moscas[i].pos.x += jogo->moscas[i].vel.x;
            if (jogo->moscas[i].pos.x <= 20.0f || jogo->moscas[i].pos.x >= 750.0f) {
                jogo->moscas[i].vel.x *= -1;
            }
        }

        Rectangle rectMosca = { jogo->moscas[i].pos.x, jogo->moscas[i].pos.y, 30, 30 };
        if (CheckCollisionRecs(rectSapo, rectMosca)) {
            jogo->moscas[i].ativa = false;
            jogo->sapo.pontos++;
            snprintf(jogo->msgStatus, sizeof(jogo->msgStatus), "Moscas Capturadas: %d", jogo->sapo.pontos);
        }
    }
}

void DesenharJogo(const Jogo *jogo) {
    if (jogo->estado == ESTADO_MENU) {
        DrawText("JOGO DO SAPO", 260, 150, 40, DARKGREEN);
        DrawText("1. Modo Vertical (Subida Infinita)", 230, 250, 20, DARKGRAY);
        DrawText("2. Modo Horizontal (Aventura)", 230, 290, 20, DARKGRAY);
        DrawText("3. Ver Ranking (Ordenado / Arquivo)", 230, 330, 20, DARKGRAY);
        return;
    }

    if (jogo->estado == ESTADO_RANKING) {
        DrawText("RANKING DE PONTUACOES", 220, 80, 30, DARKGREEN);
        for (int i = 0; i < jogo->numScores && i < 8; i++) {
            char buffer[50];
            snprintf(buffer, sizeof(buffer), "%d. %d pontos", i + 1, jogo->scores[i]);
            DrawText(buffer, 320, 150 + (i * 35), 20, DARKGRAY);
        }
        DrawText("Pressione ENTER para voltar ao Menu", 230, 500, 18, GRAY);
        return;
    }

    DrawRectangleGradientV(0, 0, 800, 600, 
                          (Color){ 100, 180, 240, 255 }, 
                          (Color){ 210, 235, 250, 255 });

    for (int l = 0; l < GRID_LINHAS; l++) {
        for (int c = 0; c < GRID_COLUNAS; c++) {
            if (jogo->bgGrid[l][c] == 1) {
                DrawRectangle(c * 80 - (int)(jogo->camX * 0.2f) % 80, l * 60, 80, 60, (Color){ 255, 255, 255, 10 });
            }
        }
    }

    float solY = 180.0f - jogo->camY * 0.05f;
    DrawCircle(680 - (int)(jogo->camX * 0.1f), (int)solY, 55, (Color){ 255, 250, 200, 180 });
    DrawCircle(680 - (int)(jogo->camX * 0.1f), (int)solY, 42, (Color){ 255, 255, 230, 230 });

    float tempo = (float)GetTime();
    
    float nuvem1X = (int)(tempo * 15.0f - jogo->camX * 0.2f) % 950 - 100;
    float nuvem1Y = 120.0f - jogo->camY * 0.1f;
    DrawCircle((int)nuvem1X, (int)nuvem1Y, 25, (Color){ 255, 255, 255, 200 });
    DrawCircle((int)nuvem1X + 20, (int)nuvem1Y - 10, 30, (Color){ 255, 255, 255, 200 });
    DrawCircle((int)nuvem1X + 45, (int)nuvem1Y, 22, (Color){ 255, 255, 255, 200 });

    float nuvem2X = (int)(tempo * 10.0f + 400 - jogo->camX * 0.3f) % 1000 - 150;
    float nuvem2Y = 200.0f - jogo->camY * 0.12f;
    DrawCircle((int)nuvem2X, (int)nuvem2Y, 35, (Color){ 255, 255, 255, 170 });
    DrawCircle((int)nuvem2X + 30, (int)nuvem2Y - 15, 45, (Color){ 255, 255, 255, 170 });
    DrawCircle((int)nuvem2X + 65, (int)nuvem2Y, 30, (Color){ 255, 255, 255, 170 });

    float montanha1Y = 480.0f - jogo->camY * 0.15f;
    DrawCircle(100 - (int)(jogo->camX * 0.15f), (int)montanha1Y + 20, 260, (Color){ 70, 130, 130, 255 });
    DrawCircle(500 - (int)(jogo->camX * 0.15f), (int)montanha1Y + 40, 320, (Color){ 60, 120, 125, 255 });

    float montanha2Y = 510.0f - jogo->camY * 0.3f;
    DrawCircle(-20 - (int)(jogo->camX * 0.25f), (int)montanha2Y, 180, (Color){ 35, 110, 65, 255 });
    DrawCircle(320 - (int)(jogo->camX * 0.25f), (int)montanha2Y + 20, 220, (Color){ 45, 125, 75, 255 });
    DrawCircle(750 - (int)(jogo->camX * 0.25f), (int)montanha2Y - 10, 200, (Color){ 30, 100, 55, 255 });

    float lagoaY = 510.0f - jogo->camY;
    if (lagoaY < 650) {
        DrawRectangleGradientV(0, (int)lagoaY, 800, 600, 
                              (Color){ 35, 135, 175, 255 }, 
                              (Color){ 15, 65, 110, 255 });

        DrawRectangle(0, (int)lagoaY - 6, 800, 8, (Color){ 195, 160, 105, 255 });

        float onda = (float)sin(tempo * 2.0) * 10.0f;
        DrawRectangle(60 + (int)onda, (int)lagoaY + 15, 120, 3, (Color){ 255, 255, 255, 120 });
        DrawRectangle(380 - (int)onda, (int)lagoaY + 28, 180, 3, (Color){ 255, 255, 255, 100 });
        DrawRectangle(200 + (int)onda, (int)lagoaY + 50, 90, 3, (Color){ 255, 255, 255, 80 });

        DrawEllipse(140, (int)lagoaY + 35, 38, 12, (Color){ 40, 140, 60, 255 });
        DrawCircle(155, (int)lagoaY + 33, 4, (Color){ 255, 220, 100, 255 });

        DrawEllipse(620, (int)lagoaY + 50, 48, 14, (Color){ 40, 140, 60, 255 });
        DrawCircle(600, (int)lagoaY + 48, 5, (Color){ 240, 150, 200, 255 });
    }

    for (int i = 0; i < jogo->numPlataformas; i++) {
        Rectangle rectDestino = jogo->plataformas[i].rect;
        rectDestino.y -= jogo->camY;
        rectDestino.x -= jogo->camX;

        if (i == 0 && jogo->estado == ESTADO_VERTICAL) {
            DrawRectangleRec(rectDestino, (Color){ 45, 150, 65, 255 });
            DrawRectangle(rectDestino.x, rectDestino.y + 12, rectDestino.width, rectDestino.height - 12, (Color){ 110, 70, 45, 255 });
            DrawRectangle(rectDestino.x, rectDestino.y + 10, rectDestino.width, 3, (Color){ 30, 110, 45, 255 });
        } else {
            DrawRectangleRec(rectDestino, jogo->plataformas[i].cor);
            DrawRectangle(rectDestino.x, rectDestino.y, rectDestino.width, 4, (Color){ 180, 120, 70, 255 });
            DrawRectangle(rectDestino.x, rectDestino.y + rectDestino.height - 3, rectDestino.width, 3, (Color){ 60, 30, 15, 255 });
        }
    }

    for (int i = 0; i < jogo->numMoscas; i++) {
        if (jogo->moscas[i].ativa) {
            Rectangle rectDestino = { 
                jogo->moscas[i].pos.x + 15.0f - jogo->camX, 
                (jogo->moscas[i].pos.y - jogo->camY) + 15.0f, 
                30, 30 
            };
            
            Texture2D texturaAtual = jogo->texMosca[jogo->moscas[i].frameAtual];
            Rectangle rectOrigem = { 0, 0, (float)texturaAtual.width, (float)texturaAtual.height };
            
            if (jogo->moscas[i].vel.x > 0) rectOrigem.width = -rectOrigem.width;

            DrawTexturePro(texturaAtual, rectOrigem, rectDestino, (Vector2){ 15.0f, 15.0f }, 0.0f, WHITE);
        }
    }

    Texture2D texAtualSapo;
    float renderSapoY = jogo->sapo.pos.y;

    if (jogo->estado == ESTADO_GAMEOVER) {
        texAtualSapo = jogo->texSapoMorto;
        renderSapoY = jogo->camY + 550.0f;
    } else {
        if (jogo->sapo.animState == SAPO_PULANDO) {
            texAtualSapo = jogo->texSapoPuloInicial;
        } else if (jogo->sapo.animState == SAPO_NO_AR) {
            texAtualSapo = jogo->texSapoNoAr;
        } else {
            texAtualSapo = jogo->texSapoBase;
        }
    }

    Rectangle rectDestinoSapo = { 
        jogo->sapo.pos.x + 25.0f - jogo->camX, 
        (renderSapoY - jogo->camY) + 25.0f, 
        50, 50 
    };
    Rectangle rectOrigemSapo = { 0, 0, (float)texAtualSapo.width, (float)texAtualSapo.height };

    if (jogo->sapo.direcao == DIR_DIREITA) rectOrigemSapo.width = -rectOrigemSapo.width;

    DrawTexturePro(texAtualSapo, rectOrigemSapo, rectDestinoSapo, (Vector2){ 25.0f, 25.0f }, 0.0f, WHITE);

    if (jogo->estado == ESTADO_GAMEOVER) {
        DrawText("GAME OVER!", 280, 220, 40, RED);
        if (jogo->tempoGameover >= 3.0f) {
            DrawText("Pressione 'R' para reiniciar ou 'M' para o Menu", 180, 300, 20, DARKGRAY);
        }
    }

    DrawRectangle(10, 10, 280, 55, (Color){ 0, 0, 0, 120 });
    DrawRectangleLines(10, 10, 280, 55, (Color){ 255, 255, 255, 80 });

    DrawText(jogo->msgStatus, 20, 16, 18, WHITE);
    
    char txtInfo[50];
    if (jogo->estado == ESTADO_VERTICAL) {
        snprintf(txtInfo, sizeof(txtInfo), "Altura: %.0fm", (400.0f - jogo->altMax) / 10.0f);
    } else {
        snprintf(txtInfo, sizeof(txtInfo), "Distancia: %.0fm", jogo->sapo.pos.x / 10.0f);
    }
    DrawText(txtInfo, 20, 38, 16, GREEN);

    DrawText("Controles: A/D ou Setas (Mover), Segurar Espaco/W (Pular continuo)", 12, 578, 15, WHITE);
}

void DestruirJogo(Jogo *jogo) {
    if (jogo == NULL) return;

    UnloadTexture(jogo->texSapoBase);
    UnloadTexture(jogo->texSapoPuloInicial);
    UnloadTexture(jogo->texSapoNoAr);
    UnloadTexture(jogo->texSapoMorto);
 
    for (int i = 0; i < 5; i++) {
        UnloadTexture(jogo->texMosca[i]);
    }

    if (jogo->plataformas != NULL) free(jogo->plataformas);
    if (jogo->moscas != NULL) free(jogo->moscas);

    free(jogo);
}