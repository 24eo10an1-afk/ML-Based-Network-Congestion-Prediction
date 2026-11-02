import sys
import joblib
import pandas as pd

model = joblib.load("early_congestion_model.pkl")

features = [
    "TrafficRate",
    "EarlyPacketsSent",
    "EarlyPacketsReceived",
    "EarlyPacketsLost",
    "EarlyDelay",
    "EarlyJitter",
    "EarlyThroughput",
    "EarlyQueue",
    "Utilization"
]

values = [float(x) for x in sys.argv[1:10]]

X = pd.DataFrame([values], columns=features)

prediction = model.predict(X)[0]

if prediction == 1:
    print("ML DECISION: CONGESTION")
    print("ACTION: REROUTE TO PATH B")
else:
    print("ML DECISION: NO CONGESTION")
    print("ACTION: KEEP CURRENT PATH")
