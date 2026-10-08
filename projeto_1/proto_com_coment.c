//Integrantes: Marcos Vinicius Aires de Medeiros, José Bernardo da Silva
#include "raylib.h"
#include "stdio.h" 
#include "stdlib.h"
#include "math.h"
#include "string.h"

// Define as dimensões da grade usada para renderizar a textura/mosaico de fundo
#define GRID_LINHAS 10
#define GRID_COLUNAS 10

// Enumerador para controlar as telas/estados do jogo
typedef enum {
    ESTADO_MENU,
    ESTADO_JOGANDO,
    ESTADO_GAMEOVER
} EstadoJogo;

// Enumerador para controlar a orientação do sprite do sapo
typedef enum {
    DIR_ESQUERDA,
    DIR_DIREITA
} Direcao;

// Estrutura para armazenar coordenadas bidimensionais (X, Y)
typedef struct {
    float x, y;
} Ponto2D;

// Estrutura que representa cada mosca coletável no jogo
typedef struct {
    Ponto2D pos;    // Posição no mundo (x, y)
    Vector2 vel;    // Velocidade de movimento
    bool ativa;     // Se a mosca ainda existe ou já foi capturada
    Color cor;      // Cor da mosca (usada como fallback/conceito)
} Mosca;

// Estrutura do jogador (Sapo)
typedef struct {
    Ponto2D pos;      // Posição atual no mapa
    Vector2 vel;      // Velocidade atual (x para horizontal, y para física de pulo/queda)
    bool noChao;      // Flag que indica se está apoiado em alguma plataforma
    Direcao direcao;  // Para onde está olhando (usado para espelhar a textura)
    int pontos;       // Quantidade de moscas capturadas
} Sapo;

// Estrutura para as plataformas de salto
typedef struct {
    Rectangle rect; // Retângulo de colisão e renderização (x, y, largura, altura)
    Color cor;      // Cor da plataforma
} Plataforma;

// Estrutura principal que guarda todo o estado global da aplicação
typedef struct {
    Sapo sapo;                  // Dados do jogador
    
    Mosca *moscas;              // Vetor dinâmico de moscas
    int numMoscas;              // Quantidade atual de moscas alocadas
    
    Plataforma *plataformas;    // Vetor dinâmico de plataformas
    int numPlataformas;         // Quantidade atual de plataformas alocadas

    EstadoJogo estado;          // Estado atual (Jogando, Game Over, etc.)
    char msgStatus[100];        // Buffer para o texto do HUD (placar)

    Texture2D texSapo;          // Textura carregada do sapo
    Texture2D texMosca;         // Textura carregada da mosca

    float camY;                 // Posição Y da câmera (deslocamento do cenário)
    float altMax;               // A maior altura (menor Y) que o sapo alcançou
    float yUltimaPlat;          // Coordenada Y da última plataforma gerada no topo

    int bgGrid[GRID_LINHAS][GRID_COLUNAS]; // Matriz para desenhar o padrão do fundo
} Jogo;

// Protótipos das Funções
Jogo* CriarJogo(int largura, int altura);
void ReiniciarJogo(Jogo *jogo);
void AtualizarJogo(Jogo *jogo);
void DesenharJogo(const Jogo *jogo);
void DestruirJogo(Jogo *jogo);
void GerarMundo(Jogo *jogo);

int main(void) {
    const int larguraTela = 800;
    const int alturaTela = 600;

    // Inicializa a janela gráfica da Raylib com a dimensão especificada
    InitWindow(larguraTela, alturaTela, "Jogo do Sapo - Subida Infinita");
    SetTargetFPS(60); // Fixa a taxa de atualização em 60 quadros por segundo

    // Aloca e inicializa a estrutura do jogo
    Jogo *jogo = CriarJogo(larguraTela, alturaTela);

    // Loop principal da aplicação (executa até o usuário fechar a janela)
    while (!WindowShouldClose()) {
        AtualizarJogo(jogo); // Processa lógica, entradas e física

        BeginDrawing();
        ClearBackground(RAYWHITE); // Limpa a tela a cada frame
        DesenharJogo(jogo);        // Desenha todos os elementos visuais
        EndDrawing();
    }

    // Libera texturas, memórias dinâmicas e encerra a janela
    DestruirJogo(jogo);
    CloseWindow();

    return 0;
}

// Aloca a memória inicial e carrega recursos essenciais do disco
Jogo* CriarJogo(int largura, int altura) {
    Jogo *jogo = (Jogo*) malloc(sizeof(Jogo));

    // Carrega imagens para as texturas
    jogo->texSapo = LoadTexture("sapo.png");
    jogo->texMosca = LoadTexture("mosca.png");
    jogo->moscas = NULL;
    jogo->plataformas = NULL;

    // Inicializa a matriz para criar um padrão xadrez suave no fundo
    for (int l = 0; l < GRID_LINHAS; l++) {
        for (int c = 0; c < GRID_COLUNAS; c++) {
            jogo->bgGrid[l][c] = (l + c) % 2;
        }
    }

    // Configura os valores iniciais do jogo
    ReiniciarJogo(jogo);
    return jogo;
}

// Reseta o estado das variáveis para iniciar ou reiniciar uma partida
void ReiniciarJogo(Jogo *jogo) {
    jogo->estado = ESTADO_JOGANDO;

    // Posição inicial do sapo
    jogo->sapo.pos.x = 375.0f;
    jogo->sapo.pos.y = 400.0f;
    jogo->sapo.vel.x = 0;
    jogo->sapo.vel.y = 0;
    jogo->sapo.noChao = false;
    jogo->sapo.direcao = DIR_DIREITA;
    jogo->sapo.pontos = 0;

    // Reseta a câmera e a altura máxima alcançada
    jogo->camY = 0.0f;
    jogo->altMax = jogo->sapo.pos.y;

    snprintf(jogo->msgStatus, sizeof(jogo->msgStatus), "Moscas Capturadas: 0");

    // Libera alocações antigas se estiver reiniciando a partida
    if (jogo->plataformas != NULL) free(jogo->plataformas);
    if (jogo->moscas != NULL) free(jogo->moscas);

    // Cria as plataformas base do início do nível
    jogo->numPlataformas = 5;
    jogo->plataformas = (Plataforma*) malloc(jogo->numPlataformas * sizeof(Plataforma));
    
    jogo->plataformas[0] = (Plataforma){ { 0, 550, 800, 50 }, DARKGREEN }; // Chão inicial
    jogo->plataformas[1] = (Plataforma){ { 300, 420, 200, 20 }, DARKBROWN };
    jogo->plataformas[2] = (Plataforma){ { 100, 300, 180, 20 }, DARKBROWN };
    jogo->plataformas[3] = (Plataforma){ { 500, 180, 180, 20 }, DARKBROWN };
    jogo->plataformas[4] = (Plataforma){ { 250, 60, 180, 20 }, DARKBROWN };

    jogo->yUltimaPlat = 60.0f; // Salva a posição Y do topo atual para continuar gerando

    // Cria as primeiras moscas
    jogo->numMoscas = 2;
    jogo->moscas = (Mosca*) malloc(jogo->numMoscas * sizeof(Mosca));

    jogo->moscas[0] = (Mosca){ { 150, 220 }, { 2.0f, 0 }, true, DARKGRAY };
    jogo->moscas[1] = (Mosca){ { 550, 100 }, { -1.5f, 0 }, true, DARKGRAY };
}

// Gera proceduralmente novas plataformas e moscas à medida que o jogador sobe
void GerarMundo(Jogo *jogo) {
    // Enquanto o topo do mapa gerado estiver próximo do topo visível da tela:
    while (jogo->yUltimaPlat > jogo->altMax - 800.0f) {
        float dy = (float)GetRandomValue(130, 180); // Distância vertical entre plataformas
        float novoY = jogo->yUltimaPlat - dy;
        float novoX = (float)GetRandomValue(50, 570);
        float largPlat = (float)GetRandomValue(160, 240);

        // Realoca a memória do vetor dinâmico para adicionar mais uma plataforma
        jogo->numPlataformas++;
        jogo->plataformas = (Plataforma*) realloc(jogo->plataformas, jogo->numPlataformas * sizeof(Plataforma));
        jogo->plataformas[jogo->numPlataformas - 1] = (Plataforma){ { novoX, novoY, largPlat, 20 }, DARKBROWN };

        jogo->yUltimaPlat = novoY;

        // 60% de chance de gerar uma mosca em cima da nova plataforma
        if (GetRandomValue(1, 100) <= 60) {
            jogo->numMoscas++;
            jogo->moscas = (Mosca*) realloc(jogo->moscas, jogo->numMoscas * sizeof(Mosca));

            float velX = (float)GetRandomValue(1, 3);
            if (GetRandomValue(0, 1) == 0) velX *= -1; // Sorteia a direção do voo (esquerda ou direita)

            jogo->moscas[jogo->numMoscas - 1] = (Mosca){ 
                { novoX + largPlat / 2.0f - 15.0f, novoY - 40.0f }, 
                { velX, 0 }, 
                true, 
                DARKGRAY 
            };
        }
    }
}

// Processa a física, movimentação, colisões e estado do jogo a cada quadro
void AtualizarJogo(Jogo *jogo) {
    // Se o jogador perdeu, aguarda a tecla 'R' para reiniciar
    if (jogo->estado == ESTADO_GAMEOVER) {
        if (IsKeyPressed(KEY_R)) ReiniciarJogo(jogo);
        return;
    }

    const float velMovimento = 6.0f;
    const float gravidade = 0.6f;
    const float forcaPulo = -20.0f;

    // --- Entradas do Usuário (Teclado) ---
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {
        jogo->sapo.pos.x -= velMovimento;
        jogo->sapo.direcao = DIR_ESQUERDA;
    }
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) {
        jogo->sapo.pos.x += velMovimento;
        jogo->sapo.direcao = DIR_DIREITA;
    }

    // Wrap das laterais da tela (Teletransporte de um lado para o outro)
    if (jogo->sapo.pos.x < -25) jogo->sapo.pos.x = 800;
    if (jogo->sapo.pos.x > 800) jogo->sapo.pos.x = -25;

    // Aplicação da gravidade e movimentação vertical
    jogo->sapo.pos.y += jogo->sapo.vel.y;
    jogo->sapo.vel.y += gravidade;

    Rectangle rectSapo = { jogo->sapo.pos.x, jogo->sapo.pos.y, 50, 50 };
    jogo->sapo.noChao = false;

    // --- Colisão com Plataformas ---
    for (int i = 0; i < jogo->numPlataformas; i++) {
        // Checa colisão apenas quando o sapo estiver caindo (vel.y >= 0)
        if (jogo->sapo.vel.y >= 0 && CheckCollisionRecs(rectSapo, jogo->plataformas[i].rect)) {
            float peAnterior = (jogo->sapo.pos.y + 50) - jogo->sapo.vel.y;
            // Valida se o sapo estava acima da plataforma no quadro anterior (evita colisão lateral/inferior)
            if (peAnterior <= jogo->plataformas[i].rect.y + 20.0f) {
                jogo->sapo.pos.y = jogo->plataformas[i].rect.y - 50; // Ajusta os pés do sapo no topo da plataforma
                jogo->sapo.vel.y = 0;
                jogo->sapo.noChao = true;
                break;
            }
        }
    }

    // Ação de Pulo
    bool pular = IsKeyDown(KEY_SPACE) || IsKeyDown(KEY_UP) || IsKeyDown(KEY_W);
    if (pular && jogo->sapo.noChao) {
        jogo->sapo.vel.y = forcaPulo;
        jogo->sapo.noChao = false;
    }

    // --- Controle da Câmera e Altura Máxima ---
    if (jogo->sapo.pos.y < jogo->altMax) {
        jogo->altMax = jogo->sapo.pos.y;
    }
    
    // A câmera só sobe se o jogador subir mais alto do que antes (rolagem de tela que não volta)
    float alvoCamY = jogo->altMax - 300.0f;
    if (alvoCamY < jogo->camY) {
        jogo->camY = alvoCamY;
    }

    // Gera novos elementos proceduralmente
    GerarMundo(jogo);

    // Condição de Game Over: caindo abaixo do limite inferior visível da câmera
    if (jogo->sapo.pos.y > jogo->camY + 650.0f) {
        jogo->estado = ESTADO_GAMEOVER;
        return;
    }

    // --- Movimento e Colisão das Moscas ---
    for (int i = 0; i < jogo->numMoscas; i++) {
        if (!jogo->moscas[i].ativa) continue;

        jogo->moscas[i].pos.x += jogo->moscas[i].vel.x;

        // Inverte a direção se rebater nas bordas laterais
        if (jogo->moscas[i].pos.x <= 20.0f || jogo->moscas[i].pos.x >= 750.0f) {
            jogo->moscas[i].vel.x *= -1;
        }

        // Detecta captura da mosca pelo sapo
        Rectangle rectMosca = { jogo->moscas[i].pos.x, jogo->moscas[i].pos.y, 30, 30 };
        if (CheckCollisionRecs(rectSapo, rectMosca)) {
            jogo->moscas[i].ativa = false;
            jogo->sapo.pontos++;
            snprintf(jogo->msgStatus, sizeof(jogo->msgStatus), "Moscas Capturadas: %d", jogo->sapo.pontos);
        }
    }
}

// Renderiza todas as camadas visuais do jogo (Fundo, Paralaxe, Elementos do jogo e HUD)
void DesenharJogo(const Jogo *jogo) {
    // Renderização da Tela de Game Over
    if (jogo->estado == ESTADO_GAMEOVER) {
        DrawText("GAME OVER!", 280, 250, 40, RED);
        DrawText("Pressione 'R' para reiniciar.", 250, 320, 20, DARKGRAY);
        return;
    }

    // --- 1. CAMADA DE FUNDO (Céu em Gradiente) ---
    DrawRectangleGradientV(0, 0, 800, 600, 
                          (Color){ 100, 180, 240, 255 }, 
                          (Color){ 210, 235, 250, 255 });

    // Desenha o padrão xadrez sutil no fundo
    for (int l = 0; l < GRID_LINHAS; l++) {
        for (int c = 0; c < GRID_COLUNAS; c++) {
            if (jogo->bgGrid[l][c] == 1) {
                DrawRectangle(c * 80, l * 60, 80, 60, (Color){ 255, 255, 255, 10 });
            }
        }
    }

    // --- 2. CAMADAS DE PARALAXE (Elementos que se movem com velocidades diferentes da câmera) ---
    // Sol no fundo
    float solY = 180.0f - jogo->camY * 0.05f;
    DrawCircle(680, (int)solY, 55, (Color){ 255, 250, 200, 180 });
    DrawCircle(680, (int)solY, 42, (Color){ 255, 255, 230, 230 });

    float tempo = (float)GetTime();
    
    // Nuvens em movimento horizontal
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

    // Montanhas ao fundo
    float montanha1Y = 480.0f - jogo->camY * 0.15f;
    DrawCircle(100, (int)montanha1Y + 20, 260, (Color){ 70, 130, 130, 255 });
    DrawCircle(500, (int)montanha1Y + 40, 320, (Color){ 60, 120, 125, 255 });

    float montanha2Y = 510.0f - jogo->camY * 0.3f;
    DrawCircle(-20, (int)montanha2Y, 180, (Color){ 35, 110, 65, 255 });
    DrawCircle(320, (int)montanha2Y + 20, 220, (Color){ 45, 125, 75, 255 });
    DrawCircle(750, (int)montanha2Y - 10, 200, (Color){ 30, 100, 55, 255 });

    // Lagoa na parte inferior (fundo inicial)
    float lagoaY = 510.0f - jogo->camY;
    if (lagoaY < 650) {
        DrawRectangleGradientV(0, (int)lagoaY, 800, 600, 
                              (Color){ 35, 135, 175, 255 }, 
                              (Color){ 15, 65, 110, 255 });

        DrawRectangle(0, (int)lagoaY - 6, 800, 8, (Color){ 195, 160, 105, 255 }); // Margem de areia

        // Animação das ondas do lago com funções trigonométricas (seno)
        float onda = (float)sin(tempo * 2.0) * 10.0f;
        DrawRectangle(60 + (int)onda, (int)lagoaY + 15, 120, 3, (Color){ 255, 255, 255, 120 });
        DrawRectangle(380 - (int)onda, (int)lagoaY + 28, 180, 3, (Color){ 255, 255, 255, 100 });
        DrawRectangle(200 + (int)onda, (int)lagoaY + 50, 90, 3, (Color){ 255, 255, 255, 80 });

        // Plantas aquáticas (Vitórias-régias)
        DrawEllipse(140, (int)lagoaY + 35, 38, 12, (Color){ 40, 140, 60, 255 });
        DrawCircle(155, (int)lagoaY + 33, 4, (Color){ 255, 220, 100, 255 });

        DrawEllipse(620, (int)lagoaY + 50, 48, 14, (Color){ 40, 140, 60, 255 });
        DrawCircle(600, (int)lagoaY + 48, 5, (Color){ 240, 150, 200, 255 });
    }

    // --- 3. DESENHO DAS PLATAFORMAS ---
    for (int i = 0; i < jogo->numPlataformas; i++) {
        Rectangle rectDestino = jogo->plataformas[i].rect;
        rectDestino.y -= jogo->camY; // Aplica o offset da câmera Y para desenhar na posição correta na tela

        if (i == 0) {
            // Desenho estilizado da plataforma inicial (chão)
            DrawRectangleRec(rectDestino, (Color){ 45, 150, 65, 255 });
            DrawRectangle(rectDestino.x, rectDestino.y + 12, rectDestino.width, rectDestino.height - 12, (Color){ 110, 70, 45, 255 });
            DrawRectangle(rectDestino.x, rectDestino.y + 10, rectDestino.width, 3, (Color){ 30, 110, 45, 255 });
        } else {
            // Desenho de bordas e grama nas plataformas normais
            DrawRectangleRec(rectDestino, jogo->plataformas[i].cor);
            DrawRectangle(rectDestino.x, rectDestino.y, rectDestino.width, 4, (Color){ 180, 120, 70, 255 });
            DrawRectangle(rectDestino.x, rectDestino.y + rectDestino.height - 3, rectDestino.width, 3, (Color){ 60, 30, 15, 255 });
        }
    }

    // --- 4. DESENHO DAS MOSCAS ---
    for (int i = 0; i < jogo->numMoscas; i++) {
        if (jogo->moscas[i].ativa) {
            Rectangle rectDestino = { 
                jogo->moscas[i].pos.x + 15.0f, 
                (jogo->moscas[i].pos.y - jogo->camY) + 15.0f, 
                30, 30 
            };
            Rectangle rectOrigem = { 0, 0, (float)jogo->texMosca.width, (float)jogo->texMosca.height };
            
            // Espelha a textura caso esteja voando para a esquerda
            if (jogo->moscas[i].vel.x < 0) rectOrigem.width = -rectOrigem.width;

            DrawTexturePro(jogo->texMosca, rectOrigem, rectDestino, (Vector2){ 15.0f, 15.0f }, 0.0f, WHITE);
        }
    }

    // --- 5. DESENHO DO JOGADOR (SAPO) ---
    // Sombra projetada nos pés quando estiver no chão
    if (jogo->sapo.noChao) {
        DrawEllipse(jogo->sapo.pos.x + 25, (jogo->sapo.pos.y - jogo->camY) + 47, 18, 5, (Color){ 0, 0, 0, 80 });
    }

    Rectangle rectDestinoSapo = { 
        jogo->sapo.pos.x + 25.0f, 
        (jogo->sapo.pos.y - jogo->camY) + 25.0f, 
        50, 50 
    };
    Rectangle rectOrigemSapo = { 0, 0, (float)jogo->texSapo.width, (float)jogo->texSapo.height };

    // Inverte a textura do sapo se estiver virado para a esquerda
    if (jogo->sapo.direcao == DIR_ESQUERDA) rectOrigemSapo.width = -rectOrigemSapo.width;

    DrawTexturePro(jogo->texSapo, rectOrigemSapo, rectDestinoSapo, (Vector2){ 25.0f, 25.0f }, 0.0f, WHITE);

    // --- 6. CAMADA DA INTERFACE DE USUÁRIO (HUD) ---
    // Painel translúcido do placar no canto superior esquerdo
    DrawRectangle(10, 10, 280, 55, (Color){ 0, 0, 0, 120 });
    DrawRectangleLines(10, 10, 280, 55, (Color){ 255, 255, 255, 80 });

    DrawText(jogo->msgStatus, 20, 16, 18, WHITE);
    
    // Cálculo e exibição da pontuação por altura alcançada
    char txtAltura[50];
    snprintf(txtAltura, sizeof(txtAltura), "Altura: %.0fm", (400.0f - jogo->altMax) / 10.0f);
    DrawText(txtAltura, 20, 38, 16, GREEN);

    // Dica de controles no rodapé
    DrawText("Controles: A/D ou Setas (Mover), Espaco/W (Pular)", 12, 578, 15, WHITE);
}

// Libera os recursos gráficos e a memória heap alocada antes de fechar o jogo
void DestruirJogo(Jogo *jogo) {
    if (jogo == NULL) return;

    // Descarrega as texturas da placa de vídeo
    UnloadTexture(jogo->texSapo);
    UnloadTexture(jogo->texMosca);

    // Libera a memória dos vetores dinâmicos
    if (jogo->plataformas != NULL) free(jogo->plataformas);
    if (jogo->moscas != NULL) free(jogo->moscas);

    // Libera a estrutura principal
    free(jogo);
}
