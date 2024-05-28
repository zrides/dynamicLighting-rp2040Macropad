// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#ifndef KEYS_H
#define KEYS_H

void initKeys();
void sendKeyboardReport();
void sendConsumerReport();

void updateCurrentLayer(int amount);
void updateVolume(int dir);

#endif