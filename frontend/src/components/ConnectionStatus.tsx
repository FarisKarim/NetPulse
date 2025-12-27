interface ConnectionStatusProps {
  connected: boolean;
}

export function ConnectionStatus({ connected }: ConnectionStatusProps) {
  return (
    <div className={`flex items-center gap-2 px-3 py-1 rounded-full text-sm ${
      connected
        ? 'bg-green-900/50 text-green-300'
        : 'bg-red-900/50 text-red-300'
    }`}>
      <span className={`w-2 h-2 rounded-full ${
        connected ? 'bg-green-400 animate-pulse' : 'bg-red-400'
      }`} />
      {connected ? 'Connected' : 'Disconnected'}
    </div>
  );
}
