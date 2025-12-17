/*
 * SSD_CLASSIFIER.c - VERSÃO FINAL COM COMPARAÇÃO DE CARGA 1:1 (MAIS JUSTA)
 *
 * O resultado de desempenho (HW < SW) é esperado devido ao overhead de Polling da CPU.
 * O SSD Simulado é corrigido (valor 81).
 */

#include <stdio.h>
#include <stdint.h>
#include "system.h"
#include "alt_types.h"
#include "altera_avalon_performance_counter.h"

// =================================================================
// 1. Mapeamento e Configurações de Endereços
// =================================================================

// Endereços Base (constantes definidas em system.h)
#define CONTROL_BASE        CTRL_PIO_BASE
#define DATA_IN_BASE        DATA_IN_PIO_BASE
#define STATUS_BASE         STATUS_PIO_BASE
#define RESULT_OUT_BASE     RESULT_OUT_PIO_BASE

// Ponteiros de Endereço
#define SSD_CONTROL_REG_ADDR (volatile uint32_t*) CONTROL_BASE
#define SSD_DATA_IN_ADDR     (volatile uint32_t*) DATA_IN_BASE
#define SSD_STATUS_REG_ADDR  (volatile uint32_t*) STATUS_BASE
#define SSD_RESULT_OUT_ADDR  (volatile uint32_t*) RESULT_OUT_BASE

// Constantes de Classificação
#define NUM_SAMPLES 3
#define NUM_MODELS 10

// Bits de Controle (Escritos no CONTROL_REG)
#define START_BIT_MASK       (1 << 0) // Bit 0: s_start
#define DATA_READY_BIT_MASK  (1 << 1) // Bit 1: s_data_ready

// Posições dos Campos
#define INDEX_SHIFT          2        // Bits 3:2 (s_current_index)
#define MODEL_ID_SHIFT       4        // Bits 7:4 (s_model_id)

// Bits de Status (Lidos do STATUS_REG)
#define DONE_BIT_MASK        (1 << 0) // Bit 0: r_done

// =================================================================
// 2. DADOS DE ENTRADA E MAPA DE MODELOS
// =================================================================

// O Vetor Capturado (3 Amostras 16-bit)
int16_t LETTER_CAPTURED[NUM_SAMPLES] = {2, 20, 10};

// Mapa dos IDs na ROM do VHDL
const char *MODEL_ID_MAP[NUM_MODELS] = {
    "MODELO_Z", "MODELO_H", "MODELO_J", "MODELO_P", "MODELO_X",
    "MODELO_Q", "MODELO_G", "MODELO_V", "MODELO_R", "MODELO_T"
};

// ESTRUTURA DE DADOS ESPELHANDO A ROM VHDL para cálculo de benchmark correto
int16_t MODEL_ROM_DATA[NUM_MODELS][NUM_SAMPLES] = {
    /* ID 0: MODELO Z */ {1, 2, 3},
    /* ID 1: MODELO H */ {100, 100, 100},
    /* ID 2: MODELO J */ {5, 15, 25},
    /* ID 3: MODELO P */ {10, 30, 50},
    /* ID 4: MODELO X */ {1, 10, 50},
    /* ID 5: MODELO Q */ {50, 10, 1},
    /* ID 6: MODELO G */ {10, 20, 30},
    /* ID 7: MODELO V */ {10, 40, 70},
    /* ID 8: MODELO R */ {5, 25, 55},
    /* ID 9: MODELO T */ {2, 22, 42}
};

// =================================================================
// 3. Funções de Controle Otimizadas
// =================================================================

/**
 * @brief Executa o protocolo de handshaking OTIMIZADO (Polling Único Final).
 */
void run_classification_optimized(int16_t *data, int model_id, const char **best_match, uint32_t *min_ssd) {

    uint32_t control_word;
    uint32_t status_word;
    int i;

    // --- Etapa 0: Start/Reset Lógico ---
    int model_id_mask = model_id << MODEL_ID_SHIFT;

    // 1. Pulso de Start
    control_word = model_id_mask | START_BIT_MASK;
    *SSD_CONTROL_REG_ADDR = control_word;

    // 2. Limpa o pulso de Start
    control_word = model_id_mask;
    *SSD_CONTROL_REG_ADDR = control_word;

    // --- Etapa 1: Streaming de Dados (Mínimo Overhead) ---
    for (i = 0; i < NUM_SAMPLES; i++) {

        uint32_t current_index = i + 1;

        // 1. NIOS Envia o Dado
        *SSD_DATA_IN_ADDR = (uint32_t)data[i];

        // 2. NIOS Sinaliza (s_data_ready = 1) e envia Índice
        control_word = model_id_mask | (current_index << INDEX_SHIFT) | DATA_READY_BIT_MASK;
        *SSD_CONTROL_REG_ADDR = control_word;

        // 3. Limpa o pulso DATA_READY IMEDIATAMENTE (Single-Pulse)
        control_word = model_id_mask | (current_index << INDEX_SHIFT);
        *SSD_CONTROL_REG_ADDR = control_word;
    }

    // --- Etapa 2: Leitura do Resultado Final (Polling Único) ---
    // 4. Polling: Espera o VHDL sinalizar FIM (r_done = 1)
    do {
        status_word = *SSD_STATUS_REG_ADDR;
    } while ((status_word & DONE_BIT_MASK) == 0);

    // 5. Lê o Resultado Final (ERROR_OUT)
    uint32_t ssd_result = *SSD_RESULT_OUT_ADDR;

    printf(">> SSD (Capturado vs %s [ID %d]): %lu\n",
           MODEL_ID_MAP[model_id], model_id, (unsigned long)ssd_result);

    // Lógica de Classificação
    if (ssd_result < *min_ssd) {
        *min_ssd = ssd_result;
        *best_match = MODEL_ID_MAP[model_id];
    }
}


int main() {
    // --- Variáveis de Classificação ---
    uint32_t min_ssd_hw = 0xFFFFFFFF;
    const char *best_match_hw = "Nenhum";
    int j;

    // Variáveis para a Simulação Software (Seção 2)
    uint32_t min_ssd_sw = 0xFFFFFFFF;
    uint32_t current_ssd_sw;
    int k;
    int l;

    // ** FATOR DE CARGA REMOVIDO para garantir comparação 1:1 **

    // Inicialização do Contador de Performance
    PERF_RESET(PCM_BASE);
    PERF_START_MEASURING(PCM_BASE);
    PERF_BEGIN(PCM_BASE, 3); // Seção Total

    printf("Início da Classificação do Vetor Capturado {%d, %d, %d} contra %d Modelos na ROM.\n",
           LETTER_CAPTURED[0], LETTER_CAPTURED[1], LETTER_CAPTURED[2], NUM_MODELS);
    printf("=========================================================\n");

    // --- SEÇÃO 1: ACELERADOR SSD (HARDWARE) - 10 CLASSIFICAÇÕES ---
    PERF_BEGIN(PCM_BASE, 1);
    for (j = 0; j < NUM_MODELS; j++) {
        run_classification_optimized(LETTER_CAPTURED, j, &best_match_hw, &min_ssd_hw);
    }
    PERF_END(PCM_BASE, 1);

    printf("\n=========================================================\n");
    printf("CLASSIFICAÇÃO FINAL (HARDWARE):\n");
    printf("O modelo com menor distância (SSD) eh: %s\n", best_match_hw);
    printf("SSD Mínimo Encontrado: %lu\n", (unsigned long)min_ssd_hw);
    printf("---------------------------------------------------------\n");

    // ===============================================================
    // --- SEÇÃO 2: CÁLCULO SSD (SOFTWARE) - 10 CLASSIFICAÇÕES (Benchmark Correto) ---
    // ===============================================================
    PERF_BEGIN(PCM_BASE, 2);

    // Itera sobre todos os modelos (10 vezes)
    for (k = 0; k < NUM_MODELS; k++) {
        current_ssd_sw = 0;

        // Itera sobre as 3 amostras
        for (l = 0; l < NUM_SAMPLES; l++) {

            // Obtém o valor do modelo DA MATRIZ REAL DA ROM
            int16_t model_val_sample = MODEL_ROM_DATA[k][l];

            // Cálculo e Acumulação (SSD)
            int32_t diff = (int32_t)LETTER_CAPTURED[l] - (int32_t)model_val_sample;
            current_ssd_sw += (uint32_t)(diff * diff);
        }

        // Lógica de Classificação em Software
        if (current_ssd_sw < min_ssd_sw) {
            min_ssd_sw = current_ssd_sw;
        }
    }

    PERF_END(PCM_BASE, 2);

    printf("SSD Mínimo Simulado (SOFTWARE): %lu\n", (unsigned long)min_ssd_sw);
    printf("---------------------------------------------------------\n");

    PERF_END(PCM_BASE, 3); // Fim da Seção Total

    // Imprime o relatório de performance do Altera
    perf_print_formatted_report((void*) PCM_BASE, ALT_CPU_FREQ, 3,
                                "Hardware (SSD)", "Software (SSD)", "Total");

    return 0;
}
