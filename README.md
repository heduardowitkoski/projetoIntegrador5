**Luva Tradutora de Libras com FPGA**

**Sobre o Projeto**:
Este é um projeto acadêmico desenvolvido para o curso de Engenharia de Computação, com o objetivo de criar um sistema embarcado capaz de reconhecer os gestos do alfabeto da Língua Brasileira de Sinais (Libras) e traduzi-los em tempo real.
O sistema utiliza uma luva equipada com sensores para capturar a flexão dos dedos e a orientação da mão. Esses dados são processados para identificar o gesto correspondente, que é então apresentado ao usuário de duas formas: através de um alto-falante que reproduz o nome da letra e em uma tela de um aplicativo para celular.

**Arquitetura e Tecnologias**:
O fluxo de funcionamento do sistema segue as seguintes etapas:
Captura: Sensores LDR e um acelerômetro, acoplados a uma luva, capturam os dados do gesto.
Coleta: Um microcontrolador ESP32 é responsável por ler os dados dos sensores e enviá-los para processamento.
Processamento: Uma FPGA (Field-Programmable Gate Array) recebe os dados e executa um algoritmo de reconhecimento de padrões em hardware para identificar a letra correspondente com alta velocidade e baixo consumo de energia.
Saída: Após o reconhecimento, o sistema aciona um módulo de áudio para a saída sonora e envia a informação para um aplicativo Android via comunicação sem fio, que exibe a letra na tela.

**Componentes Principais:**:
Hardware: ESP32, FPGA, Sensores LDR, Acelerômetro, Módulo de Áudio.
Software: Código em C/C++ para o ESP32, descrição de hardware em VHDL/Verilog para a FPGA e um aplicativo móvel para Android.
Status do Projeto
O projeto está atualmente em desenvolvimento, seguindo as etapas de arquitetura, prototipagem e implementação da solução final.
