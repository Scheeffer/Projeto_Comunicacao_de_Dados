#ifndef INTERFACE_H
#define INTERFACE_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

const char* generate_html_page(
    const char* dummy_status,
    const char* dummy,
    int adc_value,
    float velocidade,
    int valor_slider_local, // <-- Apenas o slider manual da página HTML
    int valor_node_red_freq // <-- Apenas a frequência vinda do Node-RED
)
{
    // ==================================================
    // HTML HEADER & CSS
    // ==================================================
    const char* html_header =
    "<!DOCTYPE html>"
    "<html lang='pt-BR'>"
    "<head>"
    "<meta charset='UTF-8'>"
    "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
    "<title>MONITOR CAN</title>"
    "<style>"
    "* { margin:0; padding:0; box-sizing:border-box; }"
    "body { font-family:Arial,sans-serif; background:#0f172a; color:white; min-height:100vh; }"
    ".header { padding:30px; text-align:center; background:linear-gradient(90deg,#2563eb,#06b6d4); box-shadow:0 4px 20px rgba(0,0,0,0.4); }"
    ".header h1 { font-size:42px; letter-spacing:2px; margin-bottom:10px; }"
    ".header p { font-size:16px; opacity:0.9; }"
    ".container { display:grid; grid-template-columns:repeat(auto-fit,minmax(320px,1fr)); gap:25px; padding:30px; }"
    ".card { background:#1e293b; border-radius:22px; padding:30px; box-shadow:0 8px 25px rgba(0,0,0,0.35); transition:0.3s; border:1px solid rgba(255,255,255,0.05); }"
    ".card:hover { transform:translateY(-6px); }"
    ".title { font-size:15px; text-transform:uppercase; letter-spacing:1px; color:#94a3b8; margin-bottom:20px; }"
    ".value { font-size:48px; font-weight:bold; margin-bottom:30px; }"
    ".unit { font-size:24px; opacity:0.8; }"
    ".online { color:#22c55e; font-size:32px; font-weight:bold; }"
    ".btn { width:100%; padding:18px; border:none; border-radius:16px; background:#16a34a; color:white; font-size:24px; font-weight:bold; cursor:pointer; transition:0.3s; margin-top:15px; }"
    ".btn:hover { transform:scale(1.03); }"
    ".slider-container { background:rgba(0,0,0,0.2); padding:15px; border-radius:12px; margin-bottom:20px; border:1px solid rgba(255,255,255,0.02); }"
    ".slider-label { display:flex; justify-content:space-between; font-size:14px; color:#cbd5e1; margin-bottom:8px; text-transform:uppercase; }"
    ".slider-input { width:100%; accent-color:#06b6d4; cursor:pointer; height:6px; border-radius:3px; }" 
    ".nr-display-box { background:rgba(6,182,212,0.1); padding:15px; border-radius:12px; border:1px dashed #06b6d4; margin-bottom:20px; text-align:center; }"
    ".footer { text-align:center; padding:20px; color:#94a3b8; font-size:14px; margin-top:10px; }"
    "</style>"
    "</head>"
    "<body>"
    "<div class='header'>"
    "<h1>MONITOR CAN</h1>"
    "<p>GATEWAY (MODO CONCORRENTE)</p>"
    "</div>"
    "<div class='container'>";

    // ==================================================
    // HTML FOOTER & JAVASCRIPT
    // ==================================================
    const char* html_footer =
    "</div>"
    "<div class='footer'>ESP32 • HTTP • Node-RED • IFSC</div>"
    "<script>"
    "let userIsDragging = false;"
    
    "async function atualizarDados(){"
    "try{"
    "const resposta = await fetch('/data');"
    "const dados = await resposta.json();"
    
    // CARD 1: Velocidade atual
    "if(dados.velocidade !== undefined){"
    "document.getElementById('speed').innerHTML = dados.velocidade.toFixed(1) + ' <span class=\"unit\">km/h</span>';"
    "}"
    
    // CARD 2: Slider Local 
    "if(!userIsDragging && dados.slider_local !== undefined){"
    "document.getElementById('slider_display').innerText = dados.slider_local;"
    "document.getElementById('freqSlider').value = dados.slider_local;"
    "}"
    
    // CARD 2: Referência vinda do Node-RED
    "if(dados.nr_freq !== undefined){"
    "document.getElementById('nr_value_display').innerText = dados.nr_freq;"
    "}"
    
    // CARD 3: Monitor dinâmico de quem assumiu o barramento por último
    "if(dados.modo_remoto !== undefined){"
    "const statusBox = document.getElementById('concorrenciaStatus');"
    "if(dados.modo_remoto){"
    "statusBox.innerHTML = 'Último Comando: <strong style=\"color:#06b6d4;\">NODE-RED (Via Rede)</strong>';"
    "} else {"
    "statusBox.innerHTML = 'Último Comando: <strong style=\"color:#22c55e;\">POTENCIÔMETRO (Local)</strong>';"
    "}"
    "}"
    "}catch(e){ console.log(e); }"
    "}"

    "async function enviarSliderLocal(valor){"
    "userIsDragging = false;"
    "document.getElementById('slider_display').innerText = valor;"
    "try{"
    "await fetch('/set_slider', {"
    "method: 'POST',"
    "headers: {'Content-Type': 'text/plain'},"
    "body: valor"
    "});"
    "}catch(e){ console.log('Erro ao enviar slider local:', e); }"
    "}"

    "async function toggleNodeRed(){"
    "try{"
    "const resposta = await fetch('/toggle', { method: 'POST' });"
    "const estadoTexto = await resposta.text();"
    "const estado = (estadoTexto.trim() === 'true');"
    "const btn = document.getElementById('toggleBtn');"
    "if(estado){ btn.innerHTML = 'DESLIGAR'; btn.style.background = '#dc2626'; }"
    "else { btn.innerHTML = 'LIGAR'; btn.style.background = '#16a34a'; }"
    "}catch(e){ console.log(e); }"
    "}"
    
    "atualizarDados();"
    "setInterval(atualizarDados,500);"
    "</script>"
    "</body>"
    "</html>";

    // ==================================================
    // MONTA HTML 
    // ==================================================
    size_t html_size = strlen(html_header) + strlen(html_footer) + 8000;
    char* html_page = (char*)malloc(html_size);
    if (html_page == NULL) return NULL;

    snprintf(
        html_page, html_size,
        "%s" 

        /* CARD 1: VELOCIDADE DO VEÍCULO (REDE CAN) */
        "<div class='card'>"
        "<div class='title'>VELOCIDADE ATUAL</div>"
        "<div class='value' id='speed'>%.1f <span class='unit'>km/h</span></div>"
        "<div class='title'>MONITORAMENTO REDE CAN</div>"
        "<div class='online'>ONLINE</div>"
        "<p style='margin-top:18px;font-size:16px;color:#cbd5e1;line-height:1.5;'>"
        "Barramento CAN operacional a 250 Kbps. Recebendo ID 0x4D2."
        "</p>"
        "</div>"
        
        /* CARD 2: BLOCO PROFINET */
        "<div class='card'>"
        "<div class='title' style='font-size:22px;color:white;'>PROFINET - CLP</div>"
        
        // Slider Manual Local
        "<div class='slider-container'>"
        "<div class='slider-label'>"
        "<span>Ajuste Local</span>"
        "<span><strong id='slider_display' style='color:#06b6d4; font-size:18px;'>%d</strong> Hz</span>"
        "</div>"
        "<input type='range' id='freqSlider' class='slider-input' min='0' max='60' value='%d' "
        "oninput='userIsDragging=true; document.getElementById(\"slider_display\").innerText=this.value;' "
        "onchange='enviarSliderLocal(this.value)'>"
        "</div>"

        // REFERÊNCIA REMOTA: Exibe estritamente o valor que vem do Node-RED (valor_node_red_freq)
        "<div class='nr-display-box'>"
        "<div style='font-size:11px; color:#94a3b8; text-transform:uppercase; letter-spacing:1px; margin-bottom:5px;'>Referência Freq. Node-RED</div>"
        "<div style='font-size:32px; font-weight:bold; color:#06b6d4;'><span id='nr_value_display'>%d</span> <span style='font-size:18px;'>Hz</span></div>"
        "</div>"

        "<p style='font-size:15px;color:#cbd5e1;line-height:1.5;margin-bottom:10px;'>"
        "LIGA/DESLIGA."
        "</p>"
        "<button id='toggleBtn' class='btn' onclick='toggleNodeRed()'>LIGAR</button>"
        "</div>"

        /* CARD 3: STATUS DA CONCORRÊNCIA (QUEM ALTEROU PRIMEIRO ASSUMIU) */
        "<div class='card'>"
        "<div class='title' style='font-size:22px;color:white;'>STATUS DE CONTROLE</div>"
        "<p style='font-size:15px;color:#cbd5e1;line-height:1.5;margin-bottom:20px;'>"
        "O sistema opera de forma cooperativa automática. Quem enviar uma nova instrução assume o controle do Painel E620 imediatamente."
        "</p>"
        "<div class='nr-display-box' style='background:rgba(255,255,255,0.02); border:1px solid rgba(255,255,255,0.1); margin-top:10px;'>"
        "<div id='concorrenciaStatus' style='font-size:16px; color:#cbd5e1;'>Último Comando: <strong style=\"color:#22c55e;\">POTENCIÔMETRO (Local)</strong></div>"
        "</div>"
        "</div>"
        
        "%s", 
        html_header, velocidade, valor_slider_local, valor_slider_local, valor_node_red_freq, html_footer
    );

    return html_page;
}

#endif