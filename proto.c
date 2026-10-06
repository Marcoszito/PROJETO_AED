#include "raylib.h"
#include "stdio.h" 
#include "stdlib.h"
#include "math.h"
#include "string.h"

#define GRID_LINHAS 10
#define GRID_COLUNAS 10

typedef enum {
    ESTADO_MENU,
    ESTADO_JOGANDO,
    ESTADO_GAMEOVER
} EstadoJogo;

typedef enum {
    DIR_ESQUERDA,
    DIR_DIREITA
} Direcao;

typedef struct {
    float x, y;
} Ponto2D;

typedef struct {
    Ponto2D pos;
    Vector2 vel;
    bool ativa;
    Color cor;
} Mosca;

typedef struct {
    Ponto2D pos;
    Vector2 vel;
    bool noChao;
    Direcao direcao;
    int pontos;
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

    Texture2D texSapo;
    Texture2D texMosca;

    float camY;          
    float altMax;   
    float yUltimaPlat;  

    int bgGrid[GRID_LINHAS][GRID_COLUNAS];
} Jogo;

Jogo* CriarJogo(int largura, int altura);
void ReiniciarJogo(Jogo *jogo);
void AtualizarJogo(Jogo *jogo);
void DesenharJogo(const Jogo *jogo);
void DestruirJogo(Jogo *jogo);
void GerarMundo(Jogo *jogo);

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

    jogo->texSapo = LoadTexture("sapo.png");
    jogo->texMosca = LoadTexture("mosca.png");
    jogo->moscas = NULL;
    jogo->plataformas = NULL;

    for (int l = 0; l < GRID_LINHAS; l++) {
        for (int c = 0; c < GRID_COLUNAS; c++) {
            jogo->bgGrid[l][c] = (l + c) % 2;
        }
    }

    ReiniciarJogo(jogo);
    return jogo;
}

void ReiniciarJogo(Jogo *jogo) {
    jogo->estado = ESTADO_JOGANDO;

    jogo->sapo.pos.x = 375.0f;
    jogo->sapo.pos.y = 400.0f;
    jogo->sapo.vel.x = 0;
    jogo->sapo.vel.y = 0;
    jogo->sapo.noChao = false;
    jogo->sapo.direcao = DIR_DIREITA;
    jogo->sapo.pontos = 0;

    jogo->camY = 0.0f;
    jogo->altMax = jogo->sapo.pos.y;

    snprintf(jogo->msgStatus, sizeof(jogo->msgStatus), "Moscas Capturadas: 0");

    if (jogo->plataformas != NULL) free(jogo->plataformas);
    if (jogo->moscas != NULL) free(jogo->moscas);

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

    jogo->moscas[0] = (Mosca){ { 150, 220 }, { 2.0f, 0 }, true, DARKGRAY };
    jogo->moscas[1] = (Mosca){ { 550, 100 }, { -1.5f, 0 }, true, DARKGRAY };
}

void GerarMundo(Jogo *jogo) {
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
                DARKGRAY 
            };
        }
    }
}

void AtualizarJogo(Jogo *jogo) {
    if (jogo->estado == ESTADO_GAMEOVER) {
        if (IsKeyPressed(KEY_R)) ReiniciarJogo(jogo);
        return;
    }

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

    if (jogo->sapo.pos.x < -25) jogo->sapo.pos.x = 800;
    if (jogo->sapo.pos.x > 800) jogo->sapo.pos.x = -25;

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
    }

    if (jogo->sapo.pos.y < jogo->altMax) {
        jogo->altMax = jogo->sapo.pos.y;
    }
    
    float alvoCamY = jogo->altMax - 300.0f;
    if (alvoCamY < jogo->camY) {
        jogo->camY = alvoCamY;
    }

    GerarMundo(jogo);

    if (jogo->sapo.pos.y > jogo->camY + 650.0f) {
        jogo->estado = ESTADO_GAMEOVER;
        return;
    }

    for (int i = 0; i < jogo->numMoscas; i++) {
        if (!jogo->moscas[i].ativa) continue;

        jogo->moscas[i].pos.x += jogo->moscas[i].vel.x;

        if (jogo->moscas[i].pos.x <= 20.0f || jogo->moscas[i].pos.x >= 750.0f) {
            jogo->moscas[i].vel.x *= -1;
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
    if (jogo->estado == ESTADO_GAMEOVER) {
        DrawText("GAME OVER!", 280, 250, 40, RED);
        DrawText("Pressione 'R' para reiniciar.", 250, 320, 20, DARKGRAY);
        return;
    }

    DrawRectangleGradientV(0, 0, 800, 600, 
                          (Color){ 100, 180, 240, 255 }, 
                          (Color){ 210, 235, 250, 255 });

    for (int l = 0; l < GRID_LINHAS; l++) {
        for (int c = 0; c < GRID_COLUNAS; c++) {
            if (jogo->bgGrid[l][c] == 1) {
                DrawRectangle(c * 80, l * 60, 80, 60, (Color){ 255, 255, 255, 10 });
            }
        }
    }

    float solY = 180.0f - jogo->camY * 0.05f;
    DrawCircle(680, (int)solY, 55, (Color){ 255, 250, 200, 180 });
    DrawCircle(680, (int)solY, 42, (Color){ 255, 255, 230, 230 });

    float tempo = (float)GetTime();
    
    float nuvem1X = (int)(tempo * 15.0f) % 950 - 100;
    float nuvem1Y = 120.0f - jogo->camY * 0.1f;
    DrawCircle((int)nuvem1X, (int)nuvem1Y, 25, (Color){ 255, 255, 255, 200 });
    DrawCircle((int)nuvem1X + 20, (int)nuvem1Y - 10, 30, (Color){ 255, 255, 255, 200 });
    DrawCircle((int)nuvem1X + 45, (int)nuvem1Y, 22, (Color){ 255, 255, 255, 200 });

    float nuvem2X = (int)(tempo * 10.0f + 400) % 1000 - 150;
    float nuvem2Y = 200.0f - jogo->camY * 0.12f;
    DrawCircle((int)nuvem2X, (int)nuvem2Y, 35, (Color){ 255, 255, 255, 170 });
    DrawCircle((int)nuvem2X + 30, (int)nuvem2Y - 15, 45, (Color){ 255, 255, 255, 170 });
    DrawCircle((int)nuvem2X + 65, (int)nuvem2Y, 30, (Color){ 255, 255, 255, 170 });

    float montanha1Y = 480.0f - jogo->camY * 0.15f;
    DrawCircle(100, (int)montanha1Y + 20, 260, (Color){ 70, 130, 130, 255 });
    DrawCircle(500, (int)montanha1Y + 40, 320, (Color){ 60, 120, 125, 255 });

    float montanha2Y = 510.0f - jogo->camY * 0.3f;
    DrawCircle(-20, (int)montanha2Y, 180, (Color){ 35, 110, 65, 255 });
    DrawCircle(320, (int)montanha2Y + 20, 220, (Color){ 45, 125, 75, 255 });
    DrawCircle(750, (int)montanha2Y - 10, 200, (Color){ 30, 100, 55, 255 });

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

        if (i == 0) {
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
                jogo->moscas[i].pos.x + 15.0f, 
                (jogo->moscas[i].pos.y - jogo->camY) + 15.0f, 
                30, 30 
            };
            Rectangle rectOrigem = { 0, 0, (float)jogo->texMosca.width, (float)jogo->texMosca.height };
            
            if (jogo->moscas[i].vel.x < 0) rectOrigem.width = -rectOrigem.width;

            DrawTexturePro(jogo->texMosca, rectOrigem, rectDestino, (Vector2){ 15.0f, 15.0f }, 0.0f, WHITE);
        }
    }

    if (jogo->sapo.noChao) {
        DrawEllipse(jogo->sapo.pos.x + 25, (jogo->sapo.pos.y - jogo->camY) + 47, 18, 5, (Color){ 0, 0, 0, 80 });
    }

    Rectangle rectDestinoSapo = { 
        jogo->sapo.pos.x + 25.0f, 
        (jogo->sapo.pos.y - jogo->camY) + 25.0f, 
        50, 50 
    };
    Rectangle rectOrigemSapo = { 0, 0, (float)jogo->texSapo.width, (float)jogo->texSapo.height };

    if (jogo->sapo.direcao == DIR_ESQUERDA) rectOrigemSapo.width = -rectOrigemSapo.width;

    DrawTexturePro(jogo->texSapo, rectOrigemSapo, rectDestinoSapo, (Vector2){ 25.0f, 25.0f }, 0.0f, WHITE);

    DrawRectangle(10, 10, 280, 55, (Color){ 0, 0, 0, 120 });
    DrawRectangleLines(10, 10, 280, 55, (Color){ 255, 255, 255, 80 });

    DrawText(jogo->msgStatus, 20, 16, 18, WHITE);
    
    char txtAltura[50];
    snprintf(txtAltura, sizeof(txtAltura), "Altura: %.0fm", (400.0f - jogo->altMax) / 10.0f);
    DrawText(txtAltura, 20, 38, 16, GREEN);

    DrawText("Controles: A/D ou Setas (Mover), Espaco/W (Pular)", 12, 578, 15, WHITE);
}

void DestruirJogo(Jogo *jogo) {
    if (jogo == NULL) return;

    UnloadTexture(jogo->texSapo);
    UnloadTexture(jogo->texMosca);

    if (jogo->plataformas != NULL) free(jogo->plataformas);
    if (jogo->moscas != NULL) free(jogo->moscas);

    free(jogo);
}