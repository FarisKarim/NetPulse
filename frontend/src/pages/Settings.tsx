import { useState, useEffect } from 'react';
import { useMetricsStore } from '../stores/metricsStore';
import { updateConfig, addTarget, removeTarget, fetchConfig } from '../api/http';

export function Settings() {
  const { config, targets } = useMetricsStore();

  const [probeInterval, setProbeInterval] = useState(500);
  const [probeTimeout, setProbeTimeout] = useState(1500);
  const [lossThreshold, setLossThreshold] = useState(5);
  const [p95Threshold, setP95Threshold] = useState(100);
  const [jitterThreshold, setJitterThreshold] = useState(20);
  const [saving, setSaving] = useState(false);
  const [message, setMessage] = useState('');

  // New target form
  const [newHost, setNewHost] = useState('');
  const [newPort, setNewPort] = useState(443);
  const [newLabel, setNewLabel] = useState('');
  const [addingTarget, setAddingTarget] = useState(false);

  // Load initial config
  useEffect(() => {
    if (config) {
      setProbeInterval(config.probe_interval_ms);
      setProbeTimeout(config.probe_timeout_ms);
      setLossThreshold(config.thresholds.loss_pct);
      setP95Threshold(config.thresholds.p95_ms);
      setJitterThreshold(config.thresholds.jitter_ms);
    }
  }, [config]);

  const handleSaveConfig = async () => {
    setSaving(true);
    setMessage('');

    try {
      await updateConfig({
        probe_interval_ms: probeInterval,
        probe_timeout_ms: probeTimeout,
        thresholds: {
          loss_pct: lossThreshold,
          p95_ms: p95Threshold,
          jitter_ms: jitterThreshold,
        },
      });
      setMessage('Configuration saved!');
    } catch {
      setMessage('Failed to save configuration');
    } finally {
      setSaving(false);
    }
  };

  const handleAddTarget = async () => {
    if (!newHost || !newLabel) {
      setMessage('Host and label are required');
      return;
    }

    setAddingTarget(true);
    setMessage('');

    try {
      const result = await addTarget(newHost, newPort, newLabel);
      if (result.ok) {
        setMessage(`Target "${newLabel}" added!`);
        setNewHost('');
        setNewLabel('');
        setNewPort(443);
        // Refresh config to get updated targets
        await fetchConfig();
      } else {
        setMessage(result.error || 'Failed to add target');
      }
    } catch {
      setMessage('Failed to add target');
    } finally {
      setAddingTarget(false);
    }
  };

  const handleRemoveTarget = async (targetId: string) => {
    if (!confirm('Are you sure you want to remove this target?')) {
      return;
    }

    try {
      const result = await removeTarget(targetId);
      if (result.ok) {
        setMessage('Target removed!');
      } else {
        setMessage(result.error || 'Failed to remove target');
      }
    } catch {
      setMessage('Failed to remove target');
    }
  };

  const targetList = Array.from(targets.values());

  return (
    <div className="p-6 max-w-2xl">
      <h1 className="text-2xl font-bold text-white mb-6">Settings</h1>

      {message && (
        <div className={`mb-4 p-3 rounded ${
          message.includes('Failed') || message.includes('required')
            ? 'bg-red-900/50 text-red-300'
            : 'bg-green-900/50 text-green-300'
        }`}>
          {message}
        </div>
      )}

      {/* Probe Settings */}
      <div className="bg-slate-800 rounded-lg p-4 border border-slate-700 mb-6">
        <h2 className="text-lg font-semibold text-white mb-4">Probe Settings</h2>

        <div className="space-y-4">
          <div>
            <label className="block text-slate-400 text-sm mb-1">
              Probe Interval (ms)
            </label>
            <input
              type="number"
              value={probeInterval}
              onChange={(e) => setProbeInterval(Number(e.target.value))}
              min={100}
              max={10000}
              className="w-full bg-slate-900 border border-slate-600 rounded px-3 py-2 text-white"
            />
          </div>

          <div>
            <label className="block text-slate-400 text-sm mb-1">
              Probe Timeout (ms)
            </label>
            <input
              type="number"
              value={probeTimeout}
              onChange={(e) => setProbeTimeout(Number(e.target.value))}
              min={100}
              max={30000}
              className="w-full bg-slate-900 border border-slate-600 rounded px-3 py-2 text-white"
            />
          </div>
        </div>
      </div>

      {/* Thresholds */}
      <div className="bg-slate-800 rounded-lg p-4 border border-slate-700 mb-6">
        <h2 className="text-lg font-semibold text-white mb-4">Alert Thresholds</h2>

        <div className="space-y-4">
          <div>
            <label className="block text-slate-400 text-sm mb-1">
              Packet Loss Threshold (%)
            </label>
            <input
              type="number"
              value={lossThreshold}
              onChange={(e) => setLossThreshold(Number(e.target.value))}
              min={0}
              max={100}
              step={0.1}
              className="w-full bg-slate-900 border border-slate-600 rounded px-3 py-2 text-white"
            />
          </div>

          <div>
            <label className="block text-slate-400 text-sm mb-1">
              P95 Latency Threshold (ms)
            </label>
            <input
              type="number"
              value={p95Threshold}
              onChange={(e) => setP95Threshold(Number(e.target.value))}
              min={0}
              max={10000}
              className="w-full bg-slate-900 border border-slate-600 rounded px-3 py-2 text-white"
            />
          </div>

          <div>
            <label className="block text-slate-400 text-sm mb-1">
              Jitter Threshold (ms)
            </label>
            <input
              type="number"
              value={jitterThreshold}
              onChange={(e) => setJitterThreshold(Number(e.target.value))}
              min={0}
              max={10000}
              className="w-full bg-slate-900 border border-slate-600 rounded px-3 py-2 text-white"
            />
          </div>
        </div>

        <button
          onClick={handleSaveConfig}
          disabled={saving}
          className="mt-4 bg-blue-600 hover:bg-blue-700 disabled:bg-slate-600 text-white px-4 py-2 rounded transition-colors"
        >
          {saving ? 'Saving...' : 'Save Configuration'}
        </button>
      </div>

      {/* Targets */}
      <div className="bg-slate-800 rounded-lg p-4 border border-slate-700 mb-6">
        <h2 className="text-lg font-semibold text-white mb-4">Targets</h2>

        <div className="space-y-2 mb-4">
          {targetList.map((target) => (
            <div
              key={target.id}
              className="flex justify-between items-center bg-slate-900 rounded p-3"
            >
              <div>
                <span className="text-white font-medium">{target.label}</span>
                <span className="text-slate-400 text-sm ml-2">
                  {target.host}:{target.port}
                </span>
              </div>
              <button
                onClick={() => handleRemoveTarget(target.id)}
                className="text-red-400 hover:text-red-300 text-sm"
              >
                Remove
              </button>
            </div>
          ))}
        </div>

        <div className="border-t border-slate-700 pt-4">
          <h3 className="text-sm font-medium text-slate-300 mb-3">Add New Target</h3>

          <div className="grid grid-cols-3 gap-2 mb-2">
            <input
              type="text"
              placeholder="Host (e.g., 9.9.9.9)"
              value={newHost}
              onChange={(e) => setNewHost(e.target.value)}
              className="bg-slate-900 border border-slate-600 rounded px-3 py-2 text-white text-sm"
            />
            <input
              type="number"
              placeholder="Port"
              value={newPort}
              onChange={(e) => setNewPort(Number(e.target.value))}
              min={1}
              max={65535}
              className="bg-slate-900 border border-slate-600 rounded px-3 py-2 text-white text-sm"
            />
            <input
              type="text"
              placeholder="Label (e.g., Quad9)"
              value={newLabel}
              onChange={(e) => setNewLabel(e.target.value)}
              className="bg-slate-900 border border-slate-600 rounded px-3 py-2 text-white text-sm"
            />
          </div>

          <button
            onClick={handleAddTarget}
            disabled={addingTarget}
            className="bg-green-600 hover:bg-green-700 disabled:bg-slate-600 text-white px-4 py-2 rounded text-sm transition-colors"
          >
            {addingTarget ? 'Adding...' : 'Add Target'}
          </button>
        </div>
      </div>
    </div>
  );
}
