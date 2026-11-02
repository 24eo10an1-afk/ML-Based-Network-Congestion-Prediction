import sys
import joblib
import pandas as pd

model = joblib.load("congestion_model.pkl")

features = [
    "TrafficRate",
    "PacketsSent",
    "PacketsReceived",
    "PacketsLost",
    "AverageDelay",
    "Throughput"
]

values = [float(x) for x in sys.argv[1:7]]

X = pd.DataFrame([values], columns=features)

prediction = model.predict(X)[0]

print("CONGESTION" if prediction == 1 else "NO_CONGESTION")
