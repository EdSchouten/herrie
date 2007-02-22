/*
 * Copyright (c) 2006-2007 Ed Schouten <ed@fxq.nl>
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/**
 * @file scrobbler_internal.h
 * @brief Internal AudioScrobbler routines.
 */

#ifndef _SCROBBLER_INTERNAL_H_
#define _SCROBBLER_INTERNAL_H_

/**
 * @brief Local storage structure of AudioScrobbler thread.
 */
struct scrobbler_condata {
	/**
	 * @brief The username used to authenticate with AudioScrobbler,
	 *        as fetched from the application configuration, escaped
	 *        conforming to HTTP/1.1.
	 */
	char *username;
	/**
	 * @brief The password used to authenticate with AudioScrobbler,
	 *        as fetched from the application configuration.
	 */
	const char *password;
	/**
	 * @brief The challenge that is obtained from the AudioScrobbler
	 *        server after initialising a session.
	 */
	char challenge[32];
	/**
	 * @brief The response that is sent to the AudioScrobbler server
	 *        during submission, calculated from the password and
	 *        the challenge.
	 */
	char response[32];
	/**
	 * @brief The URL used to submit tracks to.
	 */
	char *url;
	/**
	 * @brief A flag indicating whether the current session is still
	 *        valid.
	 */
	int authorized;
	/**
	 * @brief The interval at which packets may be transmitted.
	 */
	int interval;
	/**
	 * @brief A flag indicating that the last song submission was
	 *        successful.
	 */
	int ok;
};

/**
 * @brief Generate a response, based on the password and challenge.
 */
void scrobbler_hash(struct scrobbler_condata *scd);
/**
 * @brief Send a handshake to the AudioScrobbler server, thus obtaining
 *        the submission URL and challenge.
 */
int scrobbler_send_handshake(struct scrobbler_condata *scd);
/**
 * @brief Submit tracks to the AudioScrobbler server in the form of a
 *        HTTP/1.1 POST string.
 */
int scrobbler_send_tracks(struct scrobbler_condata *scd, char *poststr);

#endif /* !_SCROBBLER_INTERNAL_H_ */
