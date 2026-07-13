import csv
import sys
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

def plotEquity(tag):
    eqFile = f"results/{tag}_equity.csv"
    try:
        with open(eqFile) as f:
            reader = csv.DictReader(f)
            rows = list(reader)
    except FileNotFoundError:
        print(f"  skipping {eqFile} — not found")
        return

    ts = [int(r['timestamp']) for r in rows]
    eq = [float(r['equity']) for r in rows]
    inv = [float(r['inventory']) for r in rows]

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 8), sharex=True)

    ax1.plot(ts, eq, linewidth=0.8, color='#2196F3')
    ax1.set_ylabel('Equity')
    ax1.set_title(f'Equity Curve — {tag}')
    ax1.grid(True, alpha=0.3)

    ax2.fill_between(ts, inv, alpha=0.4, color='#FF9800')
    ax2.plot(ts, inv, linewidth=0.5, color='#E65100')
    ax2.set_ylabel('Inventory')
    ax2.set_xlabel('Tick')
    ax2.set_title('Inventory Over Time')
    ax2.grid(True, alpha=0.3)

    plt.tight_layout()
    outPng = f"results/{tag}.png"
    plt.savefig(outPng, dpi=150)
    plt.close()
    print(f"  saved {outPng}")

if __name__ == '__main__':
    tags = sys.argv[1:] if len(sys.argv) > 1 else ['as_strategy', 'static_baseline']
    print("Plotting results...")
    for t in tags:
        plotEquity(t)
    print("Done.")
