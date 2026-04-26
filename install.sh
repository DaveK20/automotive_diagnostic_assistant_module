#!/bin/bash
# install.sh — Configura o Console de Comando no Raspberry Pi
# Execute como: bash install.sh

set -e
PROJ_DIR="$HOME/car_monitor"

echo "=== Console de Comando — Instalação ==="

# 1. Dependências do sistema
echo "[1/5] Instalando dependências do sistema..."
sudo apt-get update -qq
sudo apt-get install -y python3-pip python3-venv python3-dev \
     libffi-dev libssl-dev git > /dev/null

# 2. Ambiente virtual Python
echo "[2/5] Criando ambiente virtual..."
cd "$PROJ_DIR"
python3 -m venv venv
source venv/bin/activate

# 3. Pacotes Python
echo "[3/5] Instalando pacotes Python..."
pip install --upgrade pip -q
pip install -r requirements.txt -q

# 4. Pasta de dados
echo "[4/5] Criando pasta de dados..."
mkdir -p "$PROJ_DIR/data"

# 5. Serviço systemd
echo "[5/5] Configurando serviço systemd..."
# Ajusta o caminho do usuário no arquivo de serviço
SERVICE_FILE="$PROJ_DIR/car-monitor.service"
sed -i "s|/home/pi|$HOME|g" "$SERVICE_FILE"
sudo cp "$SERVICE_FILE" /etc/systemd/system/car-monitor.service
sudo systemctl daemon-reload
sudo systemctl enable car-monitor
sudo systemctl restart car-monitor

IP=$(hostname -I | awk '{print $1}')
echo ""
echo "=========================================="
echo "  Instalação concluída!"
echo "  Acesse: http://${IP}:8000"
echo ""
echo "  Comandos úteis:"
echo "    sudo systemctl status car-monitor   # ver status"
echo "    sudo journalctl -u car-monitor -f   # ver logs"
echo "    sudo systemctl restart car-monitor  # reiniciar"
echo "=========================================="