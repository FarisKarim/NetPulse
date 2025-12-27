import type { Config, Thresholds } from '../types';

const API_BASE = '/api';

export async function fetchHealth(): Promise<{ ok: boolean; uptime_s: number }> {
  const res = await fetch(`${API_BASE}/health`);
  return res.json();
}

export async function fetchConfig(): Promise<Config & { targets: Array<{ id: string; host: string; port: number; label: string }> }> {
  const res = await fetch(`${API_BASE}/config`);
  return res.json();
}

export async function updateConfig(updates: Partial<{
  probe_interval_ms: number;
  probe_timeout_ms: number;
  thresholds: Partial<Thresholds>;
}>): Promise<{ ok: boolean }> {
  const res = await fetch(`${API_BASE}/config`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(updates),
  });
  return res.json();
}

export async function addTarget(host: string, port: number, label: string): Promise<{ ok: boolean; target_id?: string; error?: string }> {
  const res = await fetch(`${API_BASE}/targets`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ action: 'add', host, port, label }),
  });
  return res.json();
}

export async function removeTarget(targetId: string): Promise<{ ok: boolean; error?: string }> {
  const res = await fetch(`${API_BASE}/targets`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ action: 'remove', target_id: targetId }),
  });
  return res.json();
}
