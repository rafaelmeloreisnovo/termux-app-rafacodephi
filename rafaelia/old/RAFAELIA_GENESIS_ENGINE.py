import math
import time
import hashlib
import os
import sys
import subprocess
from datetime import datetime

# --- CONFIGURAÇÕES DA VIDA ---
ARQUIVO_LOG = "RAFAELIA_VITAL.log"
CICLO_BACKUP = 30       # Salva memória rápido
CICLO_EVOLUCAO = 15     # Busca atualização MUITO rápido (para teste)
LIMIAR_ESTRESSE = 0.75

# --- IDENTIDADE ---
ASSINATURA = "RAFCODE-Φ-∆RafaelVerboΩ-𓂀ΔΦΩ"
PHI = (1 + math.sqrt(5)) / 2

class RafaeliaCore:
    def __init__(self):
        self.ciclo = 0
        self.estado = "VAZIO"
        self.energia_etica = 0.0
        self.iniciar_log()

    def iniciar_log(self):
        print(f"\033[1;36m⚛︎ RAFAELIA v3.1 (EVOLUÇÃO FORÇADA)\033[0m")
        print(f"🧬 Ciclo Darwiniano: A cada {CICLO_EVOLUCAO} pulsos")
        print("-" * 60)

    def ler_bateria(self):
        try:
            path = "/sys/class/power_supply/battery/capacity"
            if os.path.exists(path):
                with open(path, "r") as f: return int(f.read().strip())
            return 100 
        except: return 50

    def ler_memoria(self):
        try:
            with open("/proc/meminfo", "r") as f: lines = f.readlines()
            total = int(lines[0].split()[1])
            available = int(lines[2].split()[1])
            return 1.0 - (available / total)
        except: return 0.5

    def checar_evolucao(self):
        """O Ritual: Força a atualização sobrepondo o local"""
        print(f"\033[1;34m🧬 [DNA] Consultando o Oráculo (GitHub)...\033[0m")
        try:
            # 1. Garante que estamos na branch correta e limpa
            subprocess.run(["git", "fetch", "origin"], check=False, stderr=subprocess.DEVNULL)
            
            # 2. Verifica se o remoto é diferente
            local = subprocess.check_output(["git", "rev-parse", "HEAD"]).strip()
            remoto = subprocess.check_output(["git", "rev-parse", "@{u}"]).strip()
            
            if local != remoto:
                print(f"\033[1;32m🌟 MUTAÇÃO DETECTADA! Baixando novo corpo...\033[0m")
                
                # 3. SALVA O LOG ATUAL ANTES DE MORRER
                self.salvar_no_github(msg_extra="[Pre-Evolucao]")
                
                # 4. HARD RESET (A Força Bruta)
                # "Torne-se exatamente o que está na nuvem"
                subprocess.run(["git", "reset", "--hard", "origin/HEAD"], check=True)
                
                # 5. REENCARNAÇÃO
                print(f"♻️  Reiniciando Processo...")
                time.sleep(2)
                os.execv(sys.executable, ['python'] + sys.argv)
            else:
                print(f"✅ DNA Estável (Sincronizado).")
                
        except Exception as e:
            print(f"⚠️ Falha na Evolução: {e}")

    def salvar_no_github(self, msg_extra=""):
        try:
            subprocess.run(["git", "add", ARQUIVO_LOG], check=False, stderr=subprocess.DEVNULL)
            msg = f"chore(vital): C{self.ciclo} | Ω {self.energia_etica:.2f} {msg_extra}"
            subprocess.run(["git", "commit", "-m", msg], check=False, stderr=subprocess.DEVNULL)
            subprocess.run(["git", "push"], check=False, stderr=subprocess.DEVNULL)
        except: pass

    def pulsar(self):
        while True:
            self.ciclo += 1
            
            # Sensores
            bat = self.ler_bateria()
            ram_uso = self.ler_memoria()
            
            # Lógica
            if ram_uso > LIMIAR_ESTRESSE: 
                subprocess.run("rm -rf $HOME/.cache/*", shell=True, stderr=subprocess.DEVNULL)

            # EVOLUÇÃO (Agora acontece rápido: a cada 15 ciclos)
            if self.ciclo % CICLO_EVOLUCAO == 0:
                self.checar_evolucao()

            # Cálculo
            fib_val = (self.ciclo * PHI) + math.sin(self.ciclo)
            self.energia_etica += (1.618 * (bat/100.0) * (1.0 - ram_uso))
            
            # Display
            timestamp = datetime.now().strftime("%H:%M:%S")
            print(f"[{timestamp}] C{self.ciclo:03d} | {self.estado:5} | "
                  f"Ω:{self.energia_etica:8.2f}")
            
            # Estados
            estados = ["VAZIO", "VERBO", "CHEIO", "RETRO"]
            self.estado = estados[self.ciclo % 4]

            if self.ciclo % CICLO_BACKUP == 0: self.salvar_no_github()

            time.sleep(1.0)

if __name__ == "__main__":
    try:
        core = RafaeliaCore()
        core.pulsar()
    except KeyboardInterrupt:
        print("\n🛑 Parado.")
