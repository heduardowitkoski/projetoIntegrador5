-- ARQUIVO: SSD_ACCEL.vhd
-- DESCRIÇÃO: Acelerador para cálculo da Soma das Diferenças Quadráticas (SSD)
-- Implementa o protocolo de handshaking OTIMIZADO (Single-Pulse) com o NIOS II.
-- O NIOS II envia os 3 dados em STREAMING e espera apenas pelo r_done.

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity SSD_ACCEL is
    port (
        -- Interface do Sistema NIOS II
        clk_sys    : in std_logic;
        reset_sys  : in std_logic;
        
        -- Entradas do NIOS II (Registros de Escrita)
        CAPTURED_DATA_IN : in std_logic_vector(31 downto 0); -- Amostra atual (16 bits LSB)
        CONTROL_REG      : in std_logic_vector(31 downto 0); -- Sinais de controle e ID do Modelo/Amostra

        -- Saídas para o NIOS II (Registros de Leitura)
        ERROR_OUT : out std_logic_vector(31 downto 0);    -- Resultado SSD acumulado (32 bits)
        STATUS_REG: out std_logic_vector(31 downto 0)     -- Flags de status (Done, Data Received)
    );
end entity SSD_ACCEL;

architecture RTL of SSD_ACCEL is

    -- Configuração do Acelerador
    constant N_SAMPLES : integer := 3;
    constant N_MODELS  : integer := 10;
    constant DATA_WIDTH : integer := 16;
    constant ROM_WIDTH : integer := DATA_WIDTH * N_SAMPLES; -- 48 bits por modelo

    -- =================================================================
    -- ROM INTERNA: 10 MODELOS (Valores 16-bit, Ordem: D3 & D2 & D1)
    -- [Valores mantidos iguais ao seu código]
    -- =================================================================
    constant ALL_MODELS_ROM : std_logic_vector(N_MODELS * ROM_WIDTH - 1 downto 0) := (
        -- ID 0: MODELO Z - {1, 2, 3}
        X"0003" & X"0002" & X"0001" &
        -- ID 1: MODELO H - {100, 100, 100}
        X"0064" & X"0064" & X"0064" &
        -- ID 2: MODELO J - {5, 15, 25}
        X"0019" & X"000F" & X"0005" &
        -- ID 3: MODELO P - {10, 30, 50}
        X"0032" & X"001E" & X"000A" &
        -- ID 4: MODELO X - {1, 10, 50}
        X"0032" & X"000A" & X"0001" &
        -- ID 5: MODELO Q - {50, 10, 1}
        X"0001" & X"000A" & X"0032" &
        -- ID 6: MODELO G (MATCH PERFEITO) - {10, 20, 30}
        X"001E" & X"0014" & X"000A" &
        -- ID 7: MODELO V - {10, 40, 70}
        X"0046" & X"0028" & X"000A" &
        -- ID 8: MODELO R - {5, 25, 55}
        X"0037" & X"0019" & X"0005" &
        -- ID 9: MODELO T - {2, 22, 42}
        X"002A" & X"0016" & X"0002"
    );

    -- SINAIS DE CONTROLE E DADOS (HANDSHAKING e Mapeamento)
    signal r_sum_square_diff : unsigned(31 downto 0) := (others => '0'); -- Acumulador SSD
    signal r_done : std_logic := '0';        -- Flag: Cálculo de todas as N_SAMPLES terminado
    
    -- r_data_received: MANTIDO PARA COMPATIBILIDADE DO ENDEREÇO STATUS_REG, MAS FIXADO EM '0'
    signal r_data_received : std_logic := '0'; 

    -- Decodificação Assíncrona do CONTROL_REG
    signal s_start : std_logic;              -- CONTROL_REG(0) - Reset Lógico/Start
    signal s_data_ready : std_logic;         -- CONTROL_REG(1) - NIOS II sinaliza dado pronto
    signal s_current_index : integer range 0 to N_SAMPLES; -- CONTROL_REG(3 downto 2) - Índice da amostra (1 a 3)
    signal s_model_id : integer range 0 to N_MODELS-1;      -- CONTROL_REG(7 downto 4) - ID do Modelo (0 a 9)

    -- Sinal para detectar a BORDA DE SUBIDA do s_data_ready (Pulso)
    signal s_data_ready_prev : std_logic := '0'; 

    -- Sinais de Dados (Signed 16-bit)
    signal s_capture_val : signed(15 downto 0); -- Dado de entrada do NIOS II
    signal s_model_val_rom : signed(15 downto 0); -- Dado extraído da ROM

    -- Usado para calcular a posição inicial do bloco do modelo na ROM
    signal s_model_base_bit : integer range 0 to N_MODELS * ROM_WIDTH := 0; 
    
    -- SINAL AUXILIAR PARA CORREÇÃO DO OFFSET
    signal s_offset_bit : integer range 0 to ROM_WIDTH := 0;


    -- =================================================================
    -- LÓGICA DE CÁLCULO DE ENDEREÇO E BUSCA NA ROM (Assíncrona)
    -- =================================================================
begin

    -- Decodificação das Entradas de Controle
    s_start          <= CONTROL_REG(0);
    s_data_ready     <= CONTROL_REG(1);
    s_current_index <= to_integer(unsigned(CONTROL_REG(3 downto 2)));

    -- Inversão de Mapeamento: Mantida
    s_model_id <= N_MODELS - 1 - to_integer(unsigned(CONTROL_REG(7 downto 4)));

    s_capture_val <= signed(CAPTURED_DATA_IN(15 downto 0));

    -- Cálculo do Endereço (Byte Address)
    s_model_base_bit <= s_model_id * ROM_WIDTH;
    
    -- Mapeia index (1, 2, 3) para offset de bits (0, 16, 32)
    s_offset_bit <= (s_current_index - 1) * DATA_WIDTH when s_current_index >= 1 else 0;

    -- Busca na ROM: Início do bloco + Offset da amostra (16 bits)
    s_model_val_rom <= signed(
        ALL_MODELS_ROM(
            s_model_base_bit + s_offset_bit + (DATA_WIDTH - 1)
            downto
            s_model_base_bit + s_offset_bit
        )
    ) when (s_current_index >= 1 and s_current_index <= N_SAMPLES)
      else (others => '0'); 

    -- Saídas para o NIOS II
    ERROR_OUT <= std_logic_vector(r_sum_square_diff);
    STATUS_REG(0) <= r_done;            -- Bit 0: Done
    STATUS_REG(1) <= r_data_received;   -- Bit 1: Data Received Acknowledge (FIXADO EM '0')
    STATUS_REG(31 downto 2) <= (others => '0');

    -- =================================================================
    -- LÓGICA SEQUENCIAL (Cálculo e Handshaking)
    -- =================================================================
    process(clk_sys, reset_sys)
        variable v_diff : signed(16 downto 0);
        variable v_sq_diff : unsigned(33 downto 0);
    begin
        if reset_sys = '1' then
            -- Reset Assíncrono
            r_sum_square_diff <= (others => '0');
            r_done <= '0';
            s_data_ready_prev <= '0';

        elsif rising_edge(clk_sys) then
            s_data_ready_prev <= s_data_ready; -- Captura o estado anterior

            -- 1. Detecção de START (Reset Lógico via NIOS II)
            if s_start = '1' then
                r_sum_square_diff <= (others => '0');
                r_done <= '0';
                -- r_data_received é sempre '0'
            
            -- 2. Detecção de DADO PRONTO (BORDA DE SUBIDA de s_data_ready)
            -- Esta é a única condição que dispara o cálculo e acumulação.
            elsif s_data_ready = '1' and s_data_ready_prev = '0' then

                -- Cálculo: Diferença Quadrática
                v_diff := resize(s_capture_val, 17) - resize(s_model_val_rom, 17);
                v_sq_diff := unsigned(v_diff * v_diff);

                -- Acumulação
                r_sum_square_diff <= r_sum_square_diff + resize(v_sq_diff, r_sum_square_diff'length);
                
                -- r_data_received permanece '0'

                -- Detecção de FIM
                if s_current_index = N_SAMPLES then
                    r_done <= '1'; -- Todas as amostras processadas
                end if;
            
            -- 3. Limpeza do Flag r_done
            -- O NIOS II pode fazer o polling final (r_done = 1) e logo em seguida 
            -- mandar o pulso s_start para o próximo modelo.
            -- Não precisamos de lógica para limpar r_done aqui, pois o START fará isso.
            
            -- Garantir que r_done seja limpo se s_start for limpo antes do próximo ciclo.
            -- No nosso caso, s_start = 1 limpa tudo no início do ciclo (caso 1).
            
            end if;
        end if;
    end process;

end architecture RTL;