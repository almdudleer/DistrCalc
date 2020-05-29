//
// Created by almdudleer on 30.05.2020.
//

#ifndef PAS_MESSAGE_PROCESSING_H
#define PAS_MESSAGE_PROCESSING_H

void send_started(Unit* self);
void send_done(Unit* self);
int reply_cs(const void* self, CsRequest* request);
void handle_started(Unit* self);
void handle_done(Unit* self);
void handle_cs_reply(Unit* self, Message* in_msg);
void handle_cs_request(Unit* self, Message* in_msg);
void handle_cs_release(Unit* self);

#endif //PAS_MESSAGE_PROCESSING_H
