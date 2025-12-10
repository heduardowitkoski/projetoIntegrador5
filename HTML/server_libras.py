from flask import Flask, request, jsonify

app = Flask(__name__)

# ==================================================================================
# 1. DADOS E MODELOS (Transcrigão exata do C++)
# ==================================================================================

# Constantes de Tolerância
TAMANHO_GESTO = 30
TOLERANCIA_GESTO_Z = 3.0
TOLERANCIA_GESTO_H = 1.5
TOLERANCIA_GESTO_J = 4.25
# Adicionei tolerâncias genéricas para os outros baseados no seu código:
TOLERANCIA_GESTO_X = 4.0
TOLERANCIA_GESTO_P = 4.0
TOLERANCIA_GESTO_Q = 4.0
TOLERANCIA_GESTO_T = 4.5 
TOLERANCIA_GESTO_V = 5.0 # Faixa entre 4 e 5 no seu código
TOLERANCIA_GESTO_R = 2.0

# Modelos Capturados (Arrays copiados do seu ESP32)
MODELO_Z_X = [
  -8.65, -8.54, -6.85, -8.13, -8.09, -7.84, -8.36, -7.81, -5.98, -3.41,
  -3.13, -3.47, -2.70, -2.25, -2.09, -2.83, -4.72, -8.86, -11.41, -8.73,
  -7.57, -6.56, -5.09, -3.49, -3.52, -3.58, -5.22, -5.56, -6.12, -5.50
]

MODELO_H_X = [
  -6.74, -6.01, -6.28, -5.49, -5.86, -5.76, -6.15, -5.94, -6.37, -5.92,
  -6.85, -5.77, -4.70, -3.29, -1.76, -3.32, -1.16, -3.13, -4.70, -5.13,
  -4.69, -5.67, -5.63, -5.40, -5.02, -4.48, -4.70, -3.81, -3.53, -3.80
]

MODELO_J_X = [
  -4.97, -4.94, -4.91, -4.90, -5.40, -6.65, -1.82, -7.71, -1.57, -2.06,
  1.26, 2.29, 3.91, 4.68, 6.10, 7.17, 8.37, 9.19, 9.96, 10.15,
  8.74, 8.06, 4.28, 1.99, 1.13, -0.01, -0.34, -0.70, -0.74, -0.24
]

MODELO_P_X = [
  1.08, 2.20, 3.37, 4.36, 4.33, 5.72, 6.05, 7.13, 7.35, 9.09,
  9.84, 10.17, 10.38, 10.50, 10.95, 11.13, 11.46, 12.58, 13.25, 13.00,
  13.89, 13.90, 13.34, 13.44, 13.65, 13.49, 12.61, 12.99, 13.30, 13.01
]

MODELO_X_X = [
  -9.37, -8.12, -9.11, -8.08, -5.04, -3.53, -2.00, -1.28, -1.12, 0.22,
  -0.77, -0.38, -0.06, -0.68, -1.41, -1.41, -1.29, -1.17, -0.85, -1.48,
  -1.56, -1.46, -1.49, -2.14, -3.14, -2.56, -1.50, -0.83, -0.86, -1.16
]

MODELO_Q_X = [
  -6.82, -6.20, -5.18, -4.87, -5.43, -6.48, -8.38, -5.33, -3.03, -3.94,
  -2.07, -0.76, -0.27, 0.28, 0.59, 1.40, 2.36, 3.10, 3.76, 4.67,
  5.16, 5.03, 4.84, 4.93, 4.84, 5.08, 4.90, 4.62, 4.47, 4.72
]

MODELO_G_X = [
  -5.03, -3.76, -2.76, -2.75, -3.11, -6.17, -0.02, -4.82, -0.28, 1.99,
  4.89, 5.40, 6.82, 7.43, 8.46, 7.87, 8.08, 6.27, 4.91, 3.97,
  2.53, 2.06, 0.32, -0.64, -2.34, -1.78, -1.92, -3.61, -3.54, -3.81
]

MODELO_V_X = [
  -6.54, -6.45, -6.58, -6.92, -6.93, -7.46, -5.92, -5.40, -4.66, -4.88,
  -4.66, -3.73, -3.15, -0.79, -0.13, -0.36, -0.64, -0.06, -0.82, -0.12,
  -0.02, -3.38, -4.48, -6.00, -5.84, -5.41, -5.77, -6.90, -7.16, -6.50
]

MODELO_R_X = [
  -6.31, -6.18, -5.65, -6.24, -5.35, -6.12, -6.46, -5.98, -6.33, -5.86,
  -4.68, -6.47, -2.01, -5.59, -2.39, -3.07, -0.63, -0.67, 1.13, 1.64,
  2.50, 2.75, 2.87, 2.97, 3.27, 2.24, 1.38, 0.02, -1.60, -2.94
]

MODELO_T_X = [
  -6.42, -5.35, -4.46, -4.82, -4.32, -5.20, -4.91, -3.98, -3.97, -2.16,
  -1.85, -1.65, -0.01, 1.64, 2.48, 3.40, 4.41, 6.90, 6.00, 5.40,
  4.76, 4.50, 4.48, 4.19, 3.30, 4.39, 0.20, -1.65, -4.33, -4.20
]

# Tabela Verdade dos LDRs
# Ordem: {mindinho, anelar, médio, indicador, polegar}
ALFABETO = [
  ([0, 0, 0, 0, 1], 'A'),
  ([1, 1, 1, 1, 0], 'B'),
  ([1, 1, 1, 1, 1], 'C'),
  ([0, 0, 0, 1, 0], 'D'),
  ([1, 1, 1, 0, 1], 'F'), # Ambíguo: F, T
  ([0, 0, 1, 1, 0], 'U'),
  ([0, 0, 1, 0, 1], 'H'),
  ([1, 0, 0, 0, 0], 'I'),
  ([0, 0, 0, 1, 1], 'L'), # Ambíguo: G, L
  ([0, 1, 1, 1, 0], 'W'),
  ([0, 0, 0, 0, 0], 'S'),
  ([1, 0, 0, 0, 1], 'Y'),
]

# ==================================================================================
# 2. FUNÇÕES AUXILIARES
# ==================================================================================

def comparar_gesto(buffer_capturado, modelo):
    """
    Calcula a similaridade entre o gesto capturado e o modelo
    usando a técnica de centralização da onda (remoção de offset)
    e erro médio absoluto.
    """
    if not buffer_capturado or len(buffer_capturado) != TAMANHO_GESTO:
        return 100.0 # Valor alto de erro se dados inválidos
    
    # 1. Calcular médias (centros)
    media_captura = sum(buffer_capturado) / TAMANHO_GESTO
    media_modelo = sum(modelo) / TAMANHO_GESTO
    
    erro_total = 0.0
    
    # 2. Comparar normalizado
    for i in range(TAMANHO_GESTO):
        valor_capturado_norm = buffer_capturado[i] - media_captura
        valor_modelo_norm = modelo[i] - media_modelo
        
        erro_total += abs(valor_capturado_norm - valor_modelo_norm)
        
    return erro_total / TAMANHO_GESTO

# ==================================================================================
# 3. ROTA PRINCIPAL (API)
# ==================================================================================

@app.route('/processar_sinal', methods=['POST'])
def processar():
    try:
        dados = request.json
        if not dados:
            return jsonify({"error": "Sem dados"}), 400

        # --- Extração de Dados ---
        ldrs = dados.get('ldrs', [0,0,0,0,0])
        buffer_accel = dados.get('buffer_accel') # Pode ser None
        movimento_detectado = dados.get('movimento', False)

        print(f"Recebido -> LDRs: {ldrs} | Movimento: {movimento_detectado}")

        # --- Lógica 1: Identificar Letra Base (LDRs) ---
        letra_base = '?'
        for padrao, letra in ALFABETO:
            if ldrs == padrao:
                letra_base = letra
                break
        
        letra_final = letra_base # Começa igual a base
        
        # --- Lógica 2: Refinamento com MPU6050 ---
        # Só processa aceleração se houver movimento E um buffer válido
        if movimento_detectado and buffer_accel and len(buffer_accel) == TAMANHO_GESTO:
            
            # --- Caso I vs J ---
            if letra_base == 'I':
                erro = comparar_gesto(buffer_accel, MODELO_J_X)
                print(f"Erro J: {erro}")
                if erro < TOLERANCIA_GESTO_J:
                    letra_final = 'J'
                else:
                    letra_final = 'I'

            # --- Caso D vs Z, X, P, Q ---
            elif letra_base == 'D':
                erroZ = comparar_gesto(buffer_accel, MODELO_Z_X)
                erroX = comparar_gesto(buffer_accel, MODELO_X_X)
                erroP = comparar_gesto(buffer_accel, MODELO_P_X)
                erroQ = comparar_gesto(buffer_accel, MODELO_Q_X)
                
                print(f"Erros -> Z:{erroZ:.2f} X:{erroX:.2f} P:{erroP:.2f} Q:{erroQ:.2f}")

                menor_erro = 100.0
                vencedor = 'D'

                if erroZ < TOLERANCIA_GESTO_Z and erroZ < menor_erro:
                    menor_erro = erroZ
                    vencedor = 'Z'
                if erroX < TOLERANCIA_GESTO_X and erroX < menor_erro:
                    menor_erro = erroX
                    vencedor = 'X'
                if erroP < TOLERANCIA_GESTO_P and erroP < menor_erro:
                    menor_erro = erroP
                    vencedor = 'P'
                if erroQ < TOLERANCIA_GESTO_Q and erroQ < menor_erro:
                    menor_erro = erroQ
                    vencedor = 'Q'
                
                letra_final = vencedor

            # --- Caso C vs Ç ---
            elif letra_base == 'C':
                # No seu código C++, Ç é apenas se houver movimento forte, sem checar modelo específico
                letra_final = 'Ç'
            
            # --- Caso H vs K ---
            elif letra_base == 'H':
                erro = comparar_gesto(buffer_accel, MODELO_H_X)
                print(f"Erro H: {erro}")
                if erro < TOLERANCIA_GESTO_H:
                    letra_final = 'H'
                else:
                    letra_final = 'K' # Default se falhar reconhecimento

            # --- Caso U vs V vs R ---
            elif letra_base == 'U':
                erroV = comparar_gesto(buffer_accel, MODELO_V_X)
                erroR = comparar_gesto(buffer_accel, MODELO_R_X)
                print(f"Erros -> V:{erroV:.2f} R:{erroR:.2f}")

                menor_erro = 100.0
                vencedor = 'U'

                # Lógica específica do seu código C++
                # if (erroV < 5 && erroV > 4 ...) -> Ajustei para lógica mais simples < 5
                if erroV < TOLERANCIA_GESTO_V and erroV < menor_erro:
                    menor_erro = erroV
                    vencedor = 'V'
                
                if erroR < TOLERANCIA_GESTO_R and erroR < menor_erro:
                    menor_erro = erroR
                    vencedor = 'R'
                
                letra_final = vencedor

            # --- Caso F vs T ---
            elif letra_base == 'F':
                erroT = comparar_gesto(buffer_accel, MODELO_T_X)
                print(f"Erro T (base F): {erroT}")
                if erroT < TOLERANCIA_GESTO_T:
                    letra_final = 'T'
                else:
                    letra_final = 'F'

            # --- Caso L vs G ---
            elif letra_base == 'L':
                # No seu código C++, para L virar G, ele comparava com o modelo T
                erroT = comparar_gesto(buffer_accel, MODELO_T_X)
                print(f"Erro T (base L->G): {erroT}")
                if erroT < TOLERANCIA_GESTO_T:
                    letra_final = 'G'
                else:
                    letra_final = 'L'
            
            # --- Caso S, E, N, M, Q ---
            elif letra_base == 'S':
                # Seu código C++ dizia que não há verificação de movimento para S
                letra_final = 'S'

        # Se não houve movimento detectado, mas a letra base depende de movimento para virar outra
        # (ex: I sem movimento é I, D sem movimento é D), o letra_final já é igual ao letra_base
        
        print(f"Resultado Final: {letra_final}")
        return jsonify({"letra": letra_final})

    except Exception as e:
        print(f"Erro Interno: {e}")
        return jsonify({"letra": "?", "erro": str(e)}), 500

if __name__ == '__main__':
    # Roda o servidor acessível na rede local
    app.run(host='0.0.0.0', port=5000, debug=True)
