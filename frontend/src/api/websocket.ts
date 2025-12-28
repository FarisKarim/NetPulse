import { useMetricsStore } from '../stores/metricsStore';
import type { WSMessage } from '../types';

class WebSocketClient {
  private ws: WebSocket | null = null;
  private reconnectTimeout: number | null = null;
  private reconnectDelay = 1000;
  private maxReconnectDelay = 30000;

  connect() {
    if (this.ws?.readyState === WebSocket.OPEN) return;

    const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
    const wsUrl = `${protocol}//${window.location.host}/ws`;

    console.log('Connecting to WebSocket:', wsUrl);

    this.ws = new WebSocket(wsUrl);

    this.ws.onopen = () => {
      console.log('WebSocket connected');
      useMetricsStore.getState().setConnected(true);
      this.reconnectDelay = 1000;
    };

    this.ws.onclose = () => {
      console.log('WebSocket disconnected');
      useMetricsStore.getState().setConnected(false);
      this.scheduleReconnect();
    };

    this.ws.onerror = (error) => {
      console.error('WebSocket error:', error);
    };

    this.ws.onmessage = (event) => {
      try {
        const message = JSON.parse(event.data) as WSMessage;
        this.handleMessage(message);
      } catch (err) {
        console.error('Failed to parse WebSocket message:', err);
      }
    };
  }

  private handleMessage(message: WSMessage) {
    const store = useMetricsStore.getState();

    switch (message.type) {
      case 'snapshot':
        store.setSnapshot(message.targets, message.config);
        break;

      case 'sample':
        store.addSample(message.target_id, {
          ts: message.ts,
          rtt_ms: message.rtt_ms,
          success: message.success,
        });
        break;

      case 'metrics':
        store.updateMetrics(message.target_id, message.metrics);
        break;

      case 'event':
        store.addEvent({
          ts: message.ts,
          target_id: message.target_id,
          reason: message.reason,
          details: message.details,
        });
        break;

      case 'config_updated':
        store.updateConfig(message.config);
        break;

      case 'targets_updated':
        store.setTargets(message.targets, message.config);
        break;
    }
  }

  private scheduleReconnect() {
    if (this.reconnectTimeout) return;

    console.log(`Reconnecting in ${this.reconnectDelay}ms...`);

    this.reconnectTimeout = window.setTimeout(() => {
      this.reconnectTimeout = null;
      this.connect();
      // Exponential backoff
      this.reconnectDelay = Math.min(this.reconnectDelay * 2, this.maxReconnectDelay);
    }, this.reconnectDelay);
  }

  disconnect() {
    if (this.reconnectTimeout) {
      clearTimeout(this.reconnectTimeout);
      this.reconnectTimeout = null;
    }
    if (this.ws) {
      this.ws.close();
      this.ws = null;
    }
  }
}

export const wsClient = new WebSocketClient();
