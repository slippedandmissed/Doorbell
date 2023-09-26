import {onRequest} from "firebase-functions/v2/https";
import * as logger from "firebase-functions/logger";
import * as admin from "firebase-admin";

admin.initializeApp();


export const ring = onRequest(async (_request, response) => {
  logger.info("Ring");

  const payload = {
    title: "Ding Dong",
    body: "Someone rang the doorbell",
  };

  const tokens = (await admin.firestore().collection("fcmTokens").get())
    .docs.map((x) => x.get("fcmToken"));
  logger.info("Sending to tokens: ", {tokens});

  await admin.messaging().sendEachForMulticast({
    tokens,
    notification: payload,
    apns: {payload: {aps: {
      sound: "doorbell.wav",
    }}},
  });

  logger.info("Notifications sent");

  response.send("Ding Dong");
});
