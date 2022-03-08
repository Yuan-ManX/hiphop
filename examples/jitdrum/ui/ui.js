/*
 * Hip-Hop / High Performance Hybrid Audio Plugins
 * Copyright (C) 2021 Luciano Iam <oss@lucianoiam.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

class JITDrumExampleUI extends DISTRHO.UI {

    constructor() {
        super();

        this.addTapListener('note-1', () => this.sendNote(1, 60, 127));
        this.addTapListener('note-2', () => this.sendNote(1, 72, 127));
        this.addTapListener('note-3', () => this.sendNote(1, 84, 127));

        document.body.style.visibility = 'visible';
    }

    addTapListener(id, listener) {
        ['touchstart', 'click'].forEach((evName) => {
            document.getElementById(id).addEventListener(evName, (ev) => {
                listener();

                if (ev.cancelable) {
                    ev.preventDefault();
                }
            });
        });
    }

}
