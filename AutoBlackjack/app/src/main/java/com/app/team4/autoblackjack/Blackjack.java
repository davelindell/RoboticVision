package com.app.team4.autoblackjack;

import android.speech.tts.TextToSpeech;

import java.util.List;

        import java.util.ArrayList;
        import java.util.List;

public class Blackjack {

    public enum Card {
        S2 (2),
        S3 (3),
        S4 (4),
        S5 (5),
        S6 (6),
        S7 (7),
        S8 (8),
        S9 (9),
        S10 (10),
        SJ (10),
        SQ (10),
        SK (10),
        SA (11),
        C2 (2),
        C3 (3),
        C4 (4),
        C5 (5),
        C6 (6),
        C7 (7),
        C8 (8),
        C9 (9),
        C10 (10),
        CJ (10),
        CQ (10),
        CK (10),
        CA (11),
        H2 (2),
        H3 (3),
        H4 (4),
        H5 (5),
        H6 (6),
        H7 (7),
        H8 (8),
        H9 (9),
        H10 (10),
        HJ (10),
        HQ (10),
        HK (10),
        HA (11),
        D2 (2),
        D3 (3),
        D4 (4),
        D5 (5),
        D6 (6),
        D7 (7),
        D8 (8),
        D9 (9),
        D10 (10),
        DJ (10),
        DQ (10),
        DK (10),
        DA (11);


        private final int value1;
        Card(int value1) {
            this.value1 = value1;
        }
    }


    public static void main(String[] args) {
        List<Card> cards = new ArrayList<Card>();

        cards.add(Card.S7);
        cards.add(Card.DA);
        cards.add(Card.C5);
        //cards.add(Card.HA);
        //cards.add(Card.SA);

        decide(cards,Card.S3);
    }

    public static void decide(List<Card> cards, Card dealer){
        int card_count=0;
        int aces=0;
        for (Card c : cards){
            if(c.equals(Card.SA) || c.equals(Card.CA) || c.equals(Card.HA) || c.equals(Card.DA)){
                aces++;
                card_count=card_count+c.value1;
            }
            else card_count=card_count+c.value1;
        }
        if(aces==0){
            no_ace(card_count,dealer);
        }
        else{
            if(card_count-aces*11>10) {
                card_count=card_count-aces*10;
                no_ace(card_count,dealer);
            }
            else {
                card_count=card_count-(aces-1)*10;
                ace(card_count,dealer);
            }
        }

    }

    public static void no_ace(int card_count,Card dealer){
        if(card_count<12) hit();
        else if(card_count==12){
            if(dealer.value1<4 || dealer.value1>6) hit();
            else stand();
        }
        else if(card_count>12 && card_count<17){
            if(dealer.value1<7) stand();
            else hit();
        }
        else stand();
    }

    public static void ace(int card_count,Card dealer){
        if(card_count<18) hit();
        else if(card_count==18){
            if(dealer.value1<9) stand();
            else hit();
        }
        else stand();
    }

    public static void hit(){
        System.out.println("hit");
    }

    public static void stand(){
        System.out.println("stand");
    }

}
