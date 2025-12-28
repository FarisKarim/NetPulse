// Sample: a single probe result
export interface Sample {
  ts: number;
  rtt_ms: number;
  success: boolean;
}

// Computed metrics for a target
export interface Metrics {
  current_rtt_ms: number;
  max_rtt_ms: number;
  loss_pct: number;
  jitter_ms: number;
  p50_ms: number;
  p95_ms: number;
}

// Target with metrics and samples
export interface Target {
  id: string;
  host: string;
  port: number;
  label: string;
  metrics: Metrics;
  samples: Sample[];
}

// Thresholds for "bad minute" detection
export interface Thresholds {
  loss_pct: number;
  p95_ms: number;
  jitter_ms: number;
}

// Configuration
export interface Config {
  probe_interval_ms: number;
  probe_timeout_ms: number;
  thresholds: Thresholds;
}

// Event (bad minute detection)
export interface NetEvent {
  ts: number;
  target_id: string;
  reason: string;
  details: {
    [key: string]: number;
  };
}

// WebSocket message types
export type WSMessage =
  | { type: 'snapshot'; targets: Target[]; config: Config }
  | { type: 'sample'; target_id: string; ts: number; rtt_ms: number; success: boolean }
  | { type: 'metrics'; target_id: string; metrics: Metrics }
  | { type: 'event'; ts: number; target_id: string; reason: string; details: Record<string, number> }
  | { type: 'config_updated'; config: Config }
  | { type: 'targets_updated'; targets: Target[]; config: Config };
